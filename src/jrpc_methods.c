#include "jrpc_methods.h"

extern ctx_t ctx;


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

/*
 * Get job logs
 *
 * Params:
 * 	@id		job id
 * 	@log_type	job log type: stdout, stderr
 */
cJSON *rpc_get_job_log(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {
	cJSON *r = NULL;
	cJSON *ptr;
	list_t *pos;

	unsigned char buf[1024*128] = {}; // 128Kb buffer for file reading
	char *enc_buf = NULL;
	size_t enc_buf_size = 0;
	char *_tmp;
	char *result = (char*)malloc(0);
	size_t encoded_size = 0;
	ssize_t n = 0;
	char *result_fill_end = NULL;

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
			// we found job

			/* Get job log type from request */
			cJSON_get_item(ptr, params, "log_type", cJSON_String) {
				jrpc_set_error(jctx, JRPC_INVALID_PARAMS, "Invalid param `log_string` type. Must be string");
				return NULL;
			}

			/* set up path to job's log */
			char *path;
			if(!asprintf(&path, "/var/lib/netexec/jobs/%s/logs/%s.log", job->id, ptr->valuestring)) {
				log_error("asprintf() error: %s", strerror(errno));
				jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Internal error. See logs");
				return NULL;
			}

			int fd = open(path, O_RDONLY);
			if (fd < 0) {
				log_error("Can't open log %s: %s", path, strerror(errno));
				jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Internal error. See logs");
				free(path);
				return NULL;
			}
			free(path);
			while ((n = read(fd, buf, sizeof(buf))) > 0) {
				log_debug("read %d bytes from log", n);

				log_info("before encode");
				enc_buf = b64_encode(buf, (size_t)n);
				if (!enc_buf) {
					log_error("Can't encode buffer with Base64. size=%d", n);
					jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Internal error. See logs");
					close(fd);
					return NULL;
				}

				enc_buf_size = strlen(enc_buf);
				_tmp = (char*)realloc(result, (encoded_size + enc_buf_size)*sizeof(char));
				if (!_tmp) {
					log_error("Can't realloc buffer to size %d", encoded_size + enc_buf_size);
					jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Internal error. See logs");
					close(fd);
					return NULL;
				}
				result = _tmp;
				result_fill_end = result + encoded_size;
				_tmp = (char*)memcpy(result_fill_end, enc_buf, enc_buf_size*sizeof(char));
				if (_tmp != result_fill_end) {
					log_error("Can't copy old buffer to new");
					jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Internal error. See logs");
					free(result);
					close(fd);
					return NULL;
				}
				free(enc_buf);
				encoded_size = encoded_size + enc_buf_size;
			};
			close(fd);
			r = cJSON_CreateObject();
			result = (char*)realloc(result, encoded_size+1);
			result[encoded_size] = '\0';
			cJSON_AddStringToObject(r, "base64_data", result);
			free(result);
			return r;
		}
	}

	free(result);
	jrpc_set_error(jctx, JRPC_INTERNAL_ERROR, "Job not found");
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

			/* set up path to job dir */
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

cJSON *rpc_shutdown_server(jrpc_context *jctx, cJSON *params, cJSON *id, void *data) {
	ev_break(ctx.loop, EVBREAK_ALL);
	return cJSON_CreateString("Shutdown initiated");
}
