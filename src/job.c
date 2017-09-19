//
// Created by dk on 08.09.17.
//

#include <dirent.h>
#include <fcntl.h>
#include <zconf.h>
#include <memory.h>
#include <stdio.h>
#include <asm/errno.h>
#include <errno.h>
#include <fts.h>

#include "job.h"


/* Helper macro: free resource if ptr != NULL */
#define guard_free(ptr) \
        if (ptr) free(ptr);

void free_job(job_t *j) {
	guard_free(j->id)
	guard_free(j->name)

	/* Sequently free strings from cmd array */
	if (j->cmd)
		for (char **arg = j->cmd; *arg; arg++)
			free(*arg);

	/* Don't forget remove cmd array itself */
	guard_free(j->cmd)
	guard_free(j)
}

job_t *init_job() {
	job_t *j = malloc(sizeof(job_t));
	bzero(j, sizeof(job_t));
	return j;
}

char **init_job_cmd(int num) {
	char **c = malloc(sizeof(char *) * (num + 1));
	bzero(c, sizeof(char *) * (num + 1));
	return c;
}

job_t *load_job(char *id) {
	job_t *job = NULL;
	char *path = NULL;
	char *job_config = NULL;
	char *end_ptr = NULL;
	cJSON *root = NULL;
	cJSON *fld = NULL;

	/* Create path to job config */
	if (!asprintf(&path, "/var/lib/netexec/jobs/%s/config.json", id)) {
		log_error("asprintf() error");
		return NULL;
	}

	/* Read job config */
	job_config = file_read(path);
	if (!job_config) {
		log_error("job %s not exists", id);
		goto free_path;
	}

	/* Parse job config */
	root = cJSON_Parse_Stream(job_config, &end_ptr);
	if (!root) {
		log_error("job %s: config.json is not a valid json", id);
		goto free_read_data;
	}

	/* Job config should be json-object */
	if (root->type != cJSON_Object) {
		log_error("job %s: config.json must be object", id);
		goto free_root;
	}

/* Helper macro: get json-object item by name with type check */
#define get_item(ptr, json, name, i_type) \
        ptr = cJSON_GetObjectItem(json, name); \
        if (!(ptr) || (ptr)->type != (i_type)) { \
                log_error("job %s: can't find field `%s` in config.json or field has invalid type", id, name); \
                goto free_job; \
        }

	/* Here we can allocate memory for job */
	job = init_job();

	get_item(fld, root, "cmd", cJSON_Array);
	int sz = cJSON_GetArraySize(fld);
	job->cmd = init_job_cmd(sz);

	/* fill array with cmd args */
	cJSON *c;
	for (int i = 0; i < sz; i++) {
		c = cJSON_GetArrayItem(fld, i);
		if (!c || c->type != cJSON_String)
			goto free_job;
		job->cmd[i] = strdup(c->valuestring);
	}

	get_item(fld, root, "id", cJSON_String);
	job->id = strdup(fld->valuestring);

	get_item(fld, root, "name", cJSON_String);
	job->name = strdup(fld->valuestring);

	get_item(fld, root, "status", cJSON_Number);
	job->status = (status_t) fld->valueint;

	get_item(fld, root, "exit_code", cJSON_Number);
	job->exit_code = (int8_t) fld->valueint;

	/* Job load succesfully. Cleanup json data & path after asprintf() */
	goto free_root;

free_job:
	/* Something goes wrong while job-json parsing */
	free_job(job);
	job = NULL;
free_root:
	/* Free resources after pardsing json */
	cJSON_Delete(root);
free_read_data:
	free(job_config);
free_path:
	/* Free resources after path asprintf() */
	free(path);
	return job;
}

int get_cmd_array_size(char **array) {
	int sz = 0;
	for (char **arg = array; *arg; arg++)
		sz++;
	return sz;
}

cJSON *convert_job2json(job_t *job) {
	cJSON *j = cJSON_CreateObject();
	cJSON_AddStringToObject(j, "id", job->id);
	cJSON_AddStringToObject(j, "name", job->name);
	cJSON_AddNumberToObject(j, "status", job->status);
	cJSON_AddNumberToObject(j, "exit_code", job->exit_code);
	cJSON *cmd = cJSON_CreateStringArray((const char **) job->cmd, get_cmd_array_size(job->cmd));
	cJSON_AddItemToObject(j, "cmd", cmd);
	return j;
}

//bool is_exists_(char *id) {
//	size_t count = 0;
//	static char *path = "/var/lib/netexec";
//
//	DIR *stream = opendir(path);
//	if (!stream) {
//		perror("Couldn't open directory");
//		return -1;
//	}
//
//	int olddirfd = open(".", O_RDONLY);
//	chdir(path);
//
//	static unsigned depth = 0;
//	struct dirent entry;
//	struct dirent *result = NULL;
//	while ((readdir_r(stream, &entry, &result) == 0) && result) {
//		if (strcmp(entry.d_name, ".") == 0) continue;
//		if (strcmp(entry.d_name, "..") == 0) continue;
//		if (!strcmp(entry.d_name, id)) return true;
//	}
//
//	int fchdir_result = fchdir(olddirfd);
//	if (fchdir_result != 0)
//		perror("Could not restore current directory");
//	closedir(stream);
//
//	return false;
//}

int compare_strings(char *job_id, void *data) {
	return !strcmp(job_id, data);
}

int listdir(char *path, int (*cb)(char *name, void *_data), void *data) {
//	size_t count = 0;
//	static char *path = "/var/lib/netexec";

	DIR *stream = opendir(path);
	if (!stream) {
		perror("Couldn't open directory");
		return -100;
	}

	int olddirfd = open(".", O_RDONLY);
	chdir(path);

	struct dirent entry;
	struct dirent *result = NULL;
	int status = 0;

	while ((readdir_r(stream, &entry, &result) == 0) && result) {
		if (strcmp(entry.d_name, ".") == 0) continue;
		if (strcmp(entry.d_name, "..") == 0) continue;
		status = cb(entry.d_name, data);
		if (status) break;
	}

	int fchdir_result = fchdir(olddirfd);
	if (fchdir_result != 0)
		log_error("Could not restore current directory");
	closedir(stream);

	return status;
}

bool is_job_exists(char *id) {
	return (bool) listdir("/var/lib/netexec/jobs", compare_strings, id);
}

int save_job(job_t *job) {
	char *path;
	int status;

	/* Create path to job config dir */
	if (!asprintf(&path, "/var/lib/netexec/jobs/%s", job->id)) {
		log_error("dir asprintf() error");
		return -1;
	}
	status = file_mkdir_p(path);
	if (status && errno != EEXIST) {
		log_error("Can't create dir %s: errno=%d %d", path, errno);
		free(path);
		return -1;
	}
	/* Free :path for future use */
	free(path);

	/* Create path to job config file */
	if (!asprintf(&path, "/var/lib/netexec/jobs/%s/config.json", job->id)) {
		log_error("config asprintf() error");
		return -1;
	}

	/* Create cJSON from job_t */
	cJSON *j = convert_job2json(job);

	/* Get string representation of cJSON (config.json content) */
	char *j_str = cJSON_Print(j);

	/* We don't need ``j`` anymore */
	cJSON_Delete(j);

	/* Write j_str to file */
	if (!file_write(path, j_str, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) {
		log_error("Can't write file: %s", path);
		free(path);
		free(j_str);
		return -1;
	}

	/* Cleanup */
	free(path);
	free(j_str);
	return 0;
}

/* Load job callback for listdir() */
int load_job_cb(char *job_id, void *data) {
	job_t *job = load_job(job_id);
	if (job) {
		log_info("Load job %s", job_id);
		list_append(data, job);
	} else {
		/* break listdir due to load_job error */
		log_error("Can't load job with id %s", job_id);
		return -1;
	}
	return 0;
}

int load_all_jobs(list_t *job_list) {
	return (bool) listdir("/var/lib/netexec/jobs", load_job_cb, job_list);
}

bool is_job_in_list(list_t *job_list, char *id) {
	list_t *pos;
	list_for_each(pos, job_list) {
//		log_debug("COMPARE: %s with %s", ((job_t*)(pos->data))->id, id);
		if (!strcmp(((job_t *) (pos->data))->id, id)) {
//			log_debug("COMPARE FOUND");
			return true;
		}
	}
	return false;
}


int recursive_delete(const char *dir) {
	int ret = 0;
	FTS *ftsp = NULL;
	FTSENT *curr;

	// Cast needed (in C) because fts_open() takes a "char * const *", instead
	// of a "const char * const *", which is only allowed in C++. fts_open()
	// does not modify the argument.
	char *files[] = {(char *) dir, NULL};

	// FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
	//                in multithreaded programs
	// FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
	//                of the specified directory
	// FTS_XDEV     - Don't cross filesystem boundaries
	ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
	if (!ftsp) {
		log_error("%s: fts_open failed: %s\n", dir, strerror(errno));
		ret = -1;
		goto finish;
	}

	while ((curr = fts_read(ftsp))) {
		switch (curr->fts_info) {
			case FTS_NS:
			case FTS_DNR:
			case FTS_ERR:
				log_error("%s: fts_read error: %s\n",
					curr->fts_accpath, strerror(curr->fts_errno));
				break;

			case FTS_DC:
			case FTS_DOT:
			case FTS_NSOK:
				// Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
				// passed to fts_open()
				break;

			case FTS_D:
				// Do nothing. Need depth-first search, so directories are deleted
				// in FTS_DP
				break;

			case FTS_DP:
			case FTS_F:
			case FTS_SL:
			case FTS_SLNONE:
			case FTS_DEFAULT:
				if (remove(curr->fts_accpath) < 0) {
					log_error("%s: Failed to remove: %s\n",
						curr->fts_path, strerror(errno));
					ret = -1;
				}
				break;
			default:
				log_error("NONSENSE: reached default case value in fts_read");
		}
	}

finish:
	if (ftsp) {
		fts_close(ftsp);
	}

	return ret;
}

/*
 /vat/lib/netexec/

 jobs/
     %id/config.json
     	 custom-data/
     	      custom-file
	 logs/
	     stdout.log
	     stderr.log



 */


//uuid run_job(base64_data)

//delete_job(uuid)

//get_job(uuid)

//get_job_log(uuid, type)

//list_jobs()


/*

 Status:

 id:
 name:
 command: ['/bin/ls', '-l', '/']
 status:
 exit_code: number
 */
