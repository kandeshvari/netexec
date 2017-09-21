#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ev.h>
#include <asm/errno.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <asprintf/asprintf.h>
#include <fcntl.h>

#include "ev_handlers.h"


#define BUFFER_SIZE 1024

extern ctx_t ctx;


void free_ev_structs(struct ev_loop *loop, struct ev_io *watcher) {
	sock_data_t *socks = (sock_data_t *) watcher->data;
	ev_io_stop(loop, watcher);

	// destroy ssl context and close socket
	SSL_shutdown(socks->ssl_sock);
	SSL_shutdown(socks->ssl_sock);
	SSL_free(socks->ssl_sock);
	ERR_remove_state(0);
	ctx.total_clients--;
	close(socks->sd);

	free(watcher->data); // free sock_data structure
	free(watcher);
}

/* Read client message */
void recv_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
	char buffer[BUFFER_SIZE];
	ssize_t read_len;

//	log_debug("ENTER recv_cb");
	if (revents & EV_ERROR) {
		log_error("got invalid event");
		return;
	}

	sock_data_t *fds = watcher->data;

	// Receive message from client socket
//	read_len = recv(watcher->fd, buffer, BUFFER_SIZE, 0);
	read_len = SSL_read(fds->ssl_sock, buffer, BUFFER_SIZE);

	if (read_len < 0) {
		perror("read_len error");
		return;
	}

	log_debug("recv %lu bytes from fd: %d", read_len, watcher->fd);

	if (read_len == 0) {
		// Stop and free watcher if client socket is closing
		free_ev_structs(loop, watcher);
//		perror("peer might closing");
		ctx.total_clients--; // Decrement total_clients count
		log_info("%d client(s) connected.", ctx.total_clients);
		return;
	} else {
		/* Parse request */
		buffer[read_len] = '\0';
//		log_debug("Request: %s", buffer);
		request_t *request = parse_http_request(buffer, read_len);
		if (request && request->body) {
			if (parse_jsonrpc(request->body, request->body_length, watcher->data) == -1) {
//			if (parse_cmd(request->body, request->body_length, watcher->data) == -1) {
				free_ev_structs(loop, watcher);
			} else {
				list_t *pos;
				int is_keepalive = 0;
				list_for_each(pos, request->headers) {
					header_t *hdr = pos->data;
//					log_debug("HEADER: %s", hdr->name);
					if (hdr && !strcasecmp(hdr->name, "Connection") && !strcasecmp(hdr->value, "keep-alive")) {
						log_debug("Found keepalive header");
						is_keepalive = 1;
						break;
					}
				}
				if (!is_keepalive) {
					log_info("No keep-alive header. Closing connection");
					free_ev_structs(loop, watcher);
				}
			}
			free_request(request);
		} else {
			SSL_write(fds->ssl_sock, bad_response_headers, bad_response_headers_len);
			free_ev_structs(loop, watcher);
		}
	}

	bzero(buffer, (size_t) read_len);
}

/*
 * Accept client requests
 */
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_sd;
	struct ev_io *w_client = (struct ev_io *) malloc(sizeof(struct ev_io));
	sock_data_t *fds = malloc(sizeof(sock_data_t));

	if (EV_ERROR & revents) {
		perror("got invalid event");
		return;
	}

	// Accept client request
	client_sd = accept(watcher->fd, (struct sockaddr *) &client_addr, &client_len);

	if (client_sd < 0) {
		log_error("accept error: %s", strerror(errno));
		return;
	}
	log_debug("ACCEPT client fd: %d", client_sd);

	ctx.total_clients++; // Increment total_clients count
//	printf("Successfully connected with client.\n");
	log_info("%d client(s) connected", ctx.total_clients);

	// wrap socket with ssl
	SSL *ssl = SSL_new(ctx.ssl_ctx);
	SSL_set_fd(ssl, client_sd);
	if (SSL_accept(ssl) <= 0) {
		ERR_print_errors_fp(stderr);
		free(fds);
		free(w_client);
	} else {
		*fds = (sock_data_t) {.sd = client_sd, .ssl_sock = ssl};
		w_client->data = fds;
		// Initialize and start watcher to read client requests
		ev_io_init(w_client, recv_cb, client_sd, EV_READ);
		ev_io_start(loop, w_client);
	}
}


static void readfd_cb(EV_P_ ev_io *w, int revents) {
	ssize_t count;
	char buffer[4096];
	job_sock_t *js = (job_sock_t*)w->data;

	do {
		count = read(w->fd, buffer, sizeof(buffer));
		log_debug("XXX: read %lu bytes; events: %d from fd: %d", count, revents, w->fd);
		if (count) {
			*(buffer + count) = '\0';
			dprintf(js->fd, "%s", buffer);
		}
	} while (count == sizeof(buffer));

	/* Child has been finished. Close fd and disable handler */
	if (!count && errno == ECHILD) {
		close(js->fd);
		free(js);
		ev_io_stop(ctx.loop, w);
		log_error("Child lost");
		return;
	}

//	if (!errno) {
//		ev_io_stop(ctx.loop, w);
//		log_warn("No data. Stop watcher");
//		return;
//	}
//	log_warn("ERROR: %d", errno);

//	ev_io_start(ctx.loop, w);
}


// another callback, this time for a time-out
//static void timeout_cb(EV_P_ ev_timer *w, int revents) {
//	puts("timeout");
//	// this causes the innermost ev_run to stop iterating
//	ev_break(EV_A_ EVBREAK_ONE);
//}

static void sigchild_cb(EV_P_ ev_child *w, int revents) {
	ev_child_stop(ctx.loop, w);
	log_info("process %d exited with status %x", w->rpid, WEXITSTATUS(w->rstatus));
	list_t *pos;
	file_watcher_t *fw = NULL;

	/* find filewatcher for specific pid */
	list_for_each(pos, ctx.proc_list) {
		fw = (file_watcher_t *) pos->data;
		if (fw && fw->pid == w->rpid)
			break;
	}
	if (fw) {
		log_info("[%d] Update job and remove warcher from proc_list", fw->pid);
		job_t *job = w->data;
		job->exit_code = WEXITSTATUS(w->rstatus);
		job->status = w->rstatus ? FAILED : COMPLETE;
		log_debug("Job update: status=%s exit_code=%d", status_str[job->status], job->exit_code);

		/* Update job on disk */
		if (save_job(job))
			log_error("Job saving failed");

		/* Remove filewatcher from list */
		list_remove_item(pos, true, NULL);
	} else {
		log_error("NONCENCE! Can't find watcher in proc_list");
	}
}

static int open_log(job_t *job, char * const name) {
	char *path;

	/* Create logs dir */
	asprintf(&path, "/var/lib/netexec/jobs/%s/logs", job->id);
	int status = mkdir(path, 0750);
	if (status && errno != EEXIST) {
		log_error("Can't create log dir `%s`", path);
		free(path);
		return -1;
	}
	free(path);

	asprintf(&path, "/var/lib/netexec/jobs/%s/logs/%s", job->id, name);
	int fd = open(path, O_RDWR|O_CREAT|O_APPEND, 0640);
	if (fd == -1) {
		log_error("Can't create log file `%s`", path);
		free(path);
		return -1;
	}
	return fd;
}

file_watcher_t *spawn_process(job_t *job) {
	int stdout_pipe[2];
	int stderr_pipe[2];

	// pipes for parent to write and read
	pipe(stdout_pipe);
	pipe(stderr_pipe);

	pid_t child_pid = fork();
	if (child_pid == -1) {
		log_error("fork error");
		return NULL;
	}

	if (!child_pid) {
		if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1)
			perror("dup2 stdout");
		if (dup2(stderr_pipe[1], STDERR_FILENO) == -1)
			perror("dup2 stderr");

		/* Close fds not required by child. Also, we don't
		   want the exec'ed program to know these existed */
		int maxfd = (int) sysconf(_SC_OPEN_MAX);
		for (int fd = 3; fd < maxfd; fd++)
			close(fd);

		execv(job->cmd[0], job->cmd);
		perror("exec error");
		exit(-1);
	} else {
		file_watcher_t *fw = malloc(sizeof(file_watcher_t));
		fw->argv = job->cmd; // TODO: ??? remove

		/* close fds not required by parent */
		close(stdout_pipe[1]);
		close(stderr_pipe[1]);

		/* set up watcher for stdout */
		ev_io_init(&fw->w_stdout, readfd_cb, stdout_pipe[0], EV_READ);
		int stdout_fd = open_log(job, "stdout.log");
		if (stdout_fd == -1) {
			free(fw);
			return NULL;
		}
		job_sock_t *js = malloc(sizeof(job_sock_t));
		js->fd = stdout_fd;
		js->job = job;

		*(&fw->w_stdout.data) = js;
		ev_io_start(ctx.loop, &fw->w_stdout);

		/* set up watcher for stderr */
		ev_io_init(&fw->w_stderr, readfd_cb, stderr_pipe[0], EV_READ);
		int stderr_fd = open_log(job, "stderr.log");
		if (stderr_fd == -1) {
			close(stdout_fd);
			free(js);
			free(fw);
			return NULL;
		}
		js = malloc(sizeof(job_sock_t));
		js->fd = stderr_fd;
		js->job = job;

		*(&fw->w_stderr.data) = js;
		ev_io_start(ctx.loop, &fw->w_stderr);

		/* set event handler for catch sigchld */
		*(&fw->w_child.data) = job;
		ev_child_init (&fw->w_child, sigchild_cb, child_pid, 0);
		ev_child_start(EV_DEFAULT_ &fw->w_child);

		/* save pid */
		fw->pid = child_pid;

		/* save in list */
		list_append(ctx.proc_list, fw);
		log_info("Watchers for pid %d are set. fds: [%d, %d]", fw->pid, stdout_pipe[0], stderr_pipe[0]);
		return fw;
	}
}
