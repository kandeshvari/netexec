#pragma once

typedef enum status {
	RUNNING,
	COMPLETE,
	FAILED,
	SPAWN_FAILED
} status_t;

static char * const status_str[sizeof(status_t)] = {
	"running",
	"complete",
	"failed",
	"spawn_failed"
};

typedef struct job {
	char *id;
	char *name;
	char **cmd;
	status_t status;
	int8_t exit_code;
} job_t;

typedef struct job_sock {
	int fd;
	job_t *job;
} job_sock_t;

void free_job(job_t *j);
job_t *init_job();
char **init_job_cmd(int num);
job_t *load_job(char *id);
int save_job(job_t *job);
int listdir(char *path, int (*cb)(char *name, void *data), void *data);
int compare_strings (char *job_id, void *data);
bool is_job_exists(char *id);
int load_all_jobs(list_t *job_list);
bool is_job_in_list(list_t *job_list, char *id);

