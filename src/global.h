#pragma once

#include <ev.h>

#include "list.h"
#include "cJSON.h"
#include "ssl.h"

typedef struct {
	void *data;
	int error_code;
	char *error_message;
} jrpc_context;

typedef cJSON *(*jrpc_function)(jrpc_context *context, cJSON *params, cJSON *id, void *data);

struct jrpc_procedure {
	char *name;
	jrpc_function function;
	void *data;
	char **p_names;
	int p_count;
};

typedef struct file_watcher {
	char **argv;
	pid_t pid;
	ev_io w_stdout;
	ev_io w_stderr;
	ev_child w_child;
} file_watcher_t;

typedef struct custom_watcher_data {
	int sd;                // tcp socket
	SSL *ssl_sock;        // ssl wrapped socket
} sock_data_t;

typedef struct jsonrpc {
	int procedure_count;
	struct jrpc_procedure *procedures;
} jsonrpc_t;

typedef struct {
	list_t *proc_list;      // list of process watchers
	struct ev_loop *loop;   // main loop
	int total_clients;      // number of clients
//	ev_timer timeout_watcher;
	ev_io w_accept;         // WATCHER: accept
	SSL_CTX *ssl_ctx;       // ssl context
	jsonrpc_t jsonrpc;	// global jsonrpc context
	list_t *job_list;	// list of all jobs
} ctx_t;
