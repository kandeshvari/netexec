#pragma once

#include <wchar.h>

#include "global.h"
#include "cJSON.h"
#include "log.h"
#include "http.h"
#include "ssl.h"

#define JRPC_PARSE_ERROR (-32700)
#define JRPC_INVALID_REQUEST (-32600)
#define JRPC_METHOD_NOT_FOUND (-32601)
#define JRPC_INVALID_PARAMS (-32603)
#define JRPC_INTERNAL_ERROR (-32693)


//static int send_response(sock_data_t *fds, char *response);
//static int send_result(sock_data_t *fds, cJSON *result, cJSON *id);
//static int send_error(sock_data_t *fds, int code, char *message, cJSON *id);
//static int invoke_procedure(char *name, cJSON *params, cJSON *id, sock_data_t *fds);
//static int eval_request(cJSON *root, sock_data_t *fds);
//void jrpc_procedure_destroy(struct jrpc_procedure *procedure);
int jrpc_register_procedure(jrpc_function function_pointer, char *name, void *data, char *p_names[], int p_count);
int jrpc_deregister_procedure(char *name);
int parse_jsonrpc(char *buffer, size_t len, sock_data_t *fds);
