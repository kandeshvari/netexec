#include "jrpc_methods.h"

extern ctx_t ctx;

cJSON *exit_server(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {
	return cJSON_CreateString("Bye!");
}

cJSON *rpc_run_job(jrpc_context *jctx, cJSON *params, cJSON *id, void *data /* sock_data_t */) {
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

cJSON *rpc_list_jobs(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {

	list_t *pos;
	cJSON *r = cJSON_CreateArray();
	list_for_each(pos, ctx.job_list) {
		job_t *job = pos->data;
		cJSON_AddItemToArray(r, convert_job2json(job));
	}
	return r;
}

cJSON *rpc_get_stats(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {
	cJSON *r = cJSON_CreateObject();
	cJSON_AddNumberToObject(r, "clients", ctx.total_clients);
	cJSON_AddNumberToObject(r, "watchers", list_len(ctx.proc_list));
	cJSON_AddNumberToObject(r, "jobs", list_len(ctx.job_list));
	return r;
}

cJSON *rpc_get_job(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {

	list_t *pos;
	cJSON *r = NULL;
	cJSON *ptr;

	/* Get job id from request */
	cJSON_get_item(ptr, params, "id", cJSON_String) {
		jrpc_set_error(jctx, JRPC_INVALID_PARAMS, "Invalid param `id` type. Must be string");
		return NULL;
	}

	/* Search id in job_list */
	list_for_each(pos, ctx.job_list) {
		job_t *job = pos->data;
		if (!strcmp(job->id, ptr->valuestring))
			r = convert_job2json(job);
	}

	/* If job not found - send null as result */
	if (!r)
		r = cJSON_CreateNull();
	return r;
}

cJSON *rpc_get_job_log(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {
	cJSON *r = NULL;
	return r;
}

cJSON *rpc_delete_job(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {
	cJSON *ptr;
	list_t *pos;

	/* Get job id from request */
	cJSON_get_item(ptr, params, "id", cJSON_String) {
		jrpc_set_error(jctx, JRPC_INVALID_PARAMS, "Invalid param `id` type. Must be string");
		return NULL;
	}

	/* Search id in job_list */
	job_t *job;
	list_for_each(pos, ctx.job_list) {
		job = pos->data;
		if (!strcmp(job->id, ptr->valuestring)) {

			/* Check job is not runnig now */
			if (job->status == RUNNING) {
				jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Job in `running` state. Stop job first");
				return NULL;
			}

			/* set up path to jpb dir */
			char *path;
			if(!asprintf(&path, "/var/lib/netexec/jobs/%s", job->id)) {
				log_error("asprintf() error: %s", strerror(errno));
				jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Internal error. See logs");
				return NULL;
			}

			/* Delete job dir */
			recursive_delete(path);
			free(path);

			/* Remove job from list and free resources */
			list_remove_item(pos, true, (void(*)(void*))free_job);

			/* Return null as a Ok result */
			return cJSON_CreateNull();
		}
	}
	jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Job not found");
	return NULL;
}
