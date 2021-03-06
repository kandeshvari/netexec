#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <ev.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "global.h"
#include "log.h"
#include "job.h"
#include "ev_handlers.h"
#include "jrpc_methods.h"

#define PORT_NO 3333


// define GLOBAl context
ctx_t ctx;

//int parse_cmd(char *buffer, size_t len, sock_data_t *fds) {
//	char *sp;
//	char *token;
//	char *cmd[1024];
//
//	if (!buffer)
//		goto error;
//
//	log_debug("parse: %s [%d]", buffer, len);
//	token = strtok_r(buffer, " \n", &sp);
//	if (!token)
//		return -1;
//	log_debug("token: %s", token);
//	if (!strncmp(token, "quit", len)) {
//		log_info("QUIT");
//		// Send message bach to the client
//		SSL_write(fds->ssl_sock, response_headers, response_headers_len);
//		SSL_write(fds->ssl_sock, STR_STATUS_BYE, strlen(STR_STATUS_BYE));
//		ev_break(ctx.loop, EVBREAK_ALL);
//		return 0;
//	}
//
//	if (!strncmp(token, "exec", len)) {
//		int j;
//		char *param;
//
//		log_info("EXEC");
//
//		for (j = 0, param = sp;; j++, param = NULL) {
//			token = strtok_r(param, " \n", &sp);
//			if (token == NULL)
//				break;
//			cmd[j] = token;
////			log_debug("save: [%s]", token);
//		}
//
//		j = 0;
////		while (cmd[j])
////			log_debug(">>> [%s]", cmd[j++]);
//
//		spawn_process(cmd);
//		log_debug("XXX: WRITE OK [%d]", fds->ssl_sock);
//		SSL_write(fds->ssl_sock, response_headers, response_headers_len);
//		SSL_write(fds->ssl_sock, STR_STATUS_OK, strlen(STR_STATUS_OK));
//		return 0;
//	}
//
//error:
//	SSL_write(fds->ssl_sock, response_headers, response_headers_len);
//	SSL_write(fds->ssl_sock, STR_STATUS_UNKNOWN, strlen(STR_STATUS_UNKNOWN));
//	return -1;
//}


/* Initialize TCP server */
int server_init(int *sock) {
	struct sockaddr_in addr;

	/* Create server socket */
	if ((*sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		log_error("socket errror: %s", strerror(errno));
		return -1;
	}

	/* set REUSEADDR for socket */
	int enable = 1;
	if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		log_error("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_NO);
	addr.sin_addr.s_addr = INADDR_ANY;

	/* Bind socket to address */
	if (bind(*sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
		log_error("bind error: %s", strerror(errno));
		return -1;
	}

	/* Start listing on the socket */
	if (listen(*sock, 2) < 0) {
		log_error("listen error: %s", strerror(errno));
		return -1;
	}

	// Initialize and start a watcher to accepts client requests
	ev_io_init(&ctx.w_accept, accept_cb, *sock, EV_READ);
	ev_io_start(ctx.loop, &ctx.w_accept);

	return 0;
}

void init() {
	const rlim_t kStackSize = 64L * 1024L * 1024L;   // min stack size = 64 Mb
	struct rlimit rl;
	int result;

	result = getrlimit(RLIMIT_STACK, &rl);
	log_info("Current stack size: %d", rl.rlim_cur);
	if (result == 0) {
		if (rl.rlim_cur < kStackSize) {
			rl.rlim_cur = kStackSize;
			result = setrlimit(RLIMIT_STACK, &rl);
			if (result != 0) {
				log_error("setrlimit returned result = %d\n", result);
			}
		}
	}
	log_info("Changed stack size: %d", rl.rlim_cur);
}

int main() {
	int sd;

	init();

	// initialize context
	ctx.loop = EV_DEFAULT;
	ctx.proc_list = list_new();
	ctx.total_clients = 0;
	ctx.job_list = list_new();

	// initialize ssl context
	init_openssl();
	ctx.ssl_ctx = create_context();
	configure_context(ctx.ssl_ctx);

	/* set common language */
	setenv("LC_ALL", "C", 1);
	setenv("LC_LANG", "C", 1);

	// disable bufferization
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	log_info("Started");
	/* Load all saved jobs */
	if (load_all_jobs(ctx.job_list))
		return -1;

	/* Inialize server */
	if (server_init(&sd))
		return -1;

//	ev_timer_init(&timeout_watcher, timeout_cb, 5.5, 0.);
//	ev_timer_start(ctx.loop, &timeout_watcher);


	jrpc_register_procedure(rpc_run_job, "run_job", NULL, (char *[]) {"id", "name", "cmd"}, 3);
	jrpc_register_procedure(rpc_list_jobs, "list_jobs", NULL, (char *[]) {}, 0);
	jrpc_register_procedure(rpc_get_stats, "get_stats", NULL, (char *[]) {}, 0);
	jrpc_register_procedure(rpc_get_job, "get_job", NULL, (char *[]) {"id"}, 1);
	jrpc_register_procedure(rpc_get_job_log, "get_job_log", NULL, (char *[]) {"id", "log_type"}, 2);
	jrpc_register_procedure(rpc_delete_job, "delete_job", NULL, (char *[]) {"id"}, 1);
	jrpc_register_procedure(rpc_shutdown_server, "shutdown_server", NULL, (char *[]) {}, 0);

	// now wait for events to arrive
	ev_run(ctx.loop, 0);
	log_info("Loop stopped. Exitting...");

	log_debug("proc_list length: %d", list_len(ctx.proc_list));
	list_free(ctx.proc_list, true, NULL);

	log_debug("job_list length: %d", list_len(ctx.job_list));
	list_free(ctx.job_list, true, (void (*)(void *)) free_job);

	// TODO: move jrpc procs to static array
	jrpc_deregister_procedure("run_job");
	jrpc_deregister_procedure("list_jobs");
	jrpc_deregister_procedure("get_stats");
	jrpc_deregister_procedure("get_job");
	jrpc_deregister_procedure("get_job_log");
	jrpc_deregister_procedure("delete_job");
	jrpc_deregister_procedure("shutdown_server");

	log_debug("procedure_count: %d", ctx.jsonrpc.procedure_count);

	cleanup_openssl();
	// unloop was called, so exit
	return 0;
}


//int main1() {
//	char *X_tmp = NULL;
//	char *resultX = (char *) malloc(0);
//	X_tmp = realloc(resultX, 88);
//	bzero(X_tmp, 88);
//	printf("%x %x\n", (unsigned int) X_tmp, (unsigned int) resultX);
//	free(X_tmp);
//
//	return 0;
//}
