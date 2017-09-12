#include "jrpc_methods.h"

extern ctx_t ctx;

cJSON *exit_server(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {
	return cJSON_CreateString("Bye!");
}


cJSON *run_job(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {
//	sock_data_t *s = (sock_data_t*)data;

	cJSON *ptr;
	job_t *job = init_job();

#define get_item_wrapper(name, _type) \
	cJSON_get_item(ptr, params, name, _type) { \
		jrpc_set_error(jctx, JRPC_INVALID_PARAMS, "Invalid param type"); \
		free_job(job); \
		return NULL; \
	}

	get_item_wrapper("id", cJSON_String);
	job->id = strdup(ptr->valuestring);
	// TODO: validate id field !!!
	if (is_job_in_list(ctx.job_list, job->id)){
		log_error("job with id `%s` already exists", job->id);
		free_job(job);
		jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "job already exists");
		return NULL;
	}

	get_item_wrapper("name", cJSON_String);
	job->name = strdup(ptr->valuestring);

	get_item_wrapper("cmd", cJSON_Array);
	int sz = cJSON_GetArraySize(ptr);
	if (!sz) {
		log_error("Can't get array size for job %s", job->id);
		free_job(job);
		jrpc_set_error(jctx, JRPC_INVALID_PARAMS, "Invalid param `cmd` size. Must be at least one");
		return NULL;
	}

	job->cmd = init_job_cmd(sz);
	cJSON *c;
	for (int i=0; i<sz; i++) {
		c = cJSON_GetArrayItem(ptr, i);
		if (!c || c->type != cJSON_String) {
			free_job(job);
			jrpc_set_error(jctx, JRPC_INVALID_PARAMS, "Invalid param `cmd` type. Must be array of strings");
			return NULL;
		}
		job->cmd[i] = strdup(c->valuestring);
	}

	/* Add job to job list */
	list_append(ctx.job_list, job);

	/* Save job to disk */
	if (save_job(job)) {
		free_job(job);
		jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Can't save job to disk");
		return NULL;
	}

	/* Spawn process immediately */
	if (!spawn_process(job)) {
		jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Can't spawn process");
		job->status = SPAWN_FAILED;
		/* Update job status on disk */
		if (save_job(job))
			log_error("Can't update status for job %s", job->id);
	}

	cJSON *r = cJSON_CreateObject();
	cJSON_AddStringToObject(r, "id", job->id);

	return r;
}
