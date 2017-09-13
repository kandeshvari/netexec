#pragma once

#include <asprintf/asprintf.h>

#include "stddef.h"
#include "cJSON.h"
#include "jsonrpc.h"
#include "job.h"
#include "log.h"
#include "ev_handlers.h"


#define cJSON_get_item(ptr, json, name, i_type) \
	ptr = cJSON_GetObjectItem(json, name); \
	if (!(ptr) || (ptr)->type != (i_type))

#define jrpc_set_error(jctx, code, msg) \
	(jctx)->error_code = code; \
	(jctx)->error_message = strdup(msg);


cJSON *rpc_run_job(jrpc_context *jctx, cJSON *params, cJSON *id, void *data);
cJSON *rpc_list_jobs(jrpc_context *jctx, cJSON *params, cJSON *id, void *data);
cJSON *rpc_get_stats(jrpc_context *jctx, cJSON *params, cJSON *id, void *data);
cJSON *rpc_get_job(jrpc_context *jctx, cJSON *params, cJSON *id, void *data);
cJSON *rpc_get_job_log(jrpc_context *jctx, cJSON *params, cJSON *id, void *data);
cJSON *rpc_delete_job(jrpc_context *jctx, cJSON *params, cJSON *id, void *data);
