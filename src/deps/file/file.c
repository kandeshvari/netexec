
//
// utils.h
//
// Copyright (c) 2013 TJ Holowaychuk <tj@vision-media.ca>
//

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <asm/errno.h>
#include <errno.h>

#include "file.h"
#include "../log.h"

/*
 * Return the filesize of `filename` or -1.
 */

off_t
file_size(const char *filename) {
	struct stat s;
	if (stat(filename, &s) < 0) return -1;
	return s.st_size;
}

/*
 * Check if `filename` exists.
 */

int
file_exists(const char *filename) {
	return file_size(filename) != -1;
}

/*
 * Read the contents of `filename` or return NULL.
 */

char *
file_read(const char *filename) {
	off_t len = file_size(filename);
	if (len < 0) return NULL;

	char *buf = malloc(len + 1);
	if (!buf) return NULL;

	int fd = open(filename, O_RDONLY);
	if (fd < 0) return NULL;

	ssize_t size = read(fd, buf, len);
	close(fd);

	if (size != len) return NULL;

	return buf;
}

size_t file_write(const char *filename, char *buf, int flags, int mode) {

	size_t len = strlen(buf);
	log_debug("json len %d", len);
	int fd = open(filename, flags, mode);
	log_debug("json fd %d, [%d]", fd, errno);
	if (fd < 0) return 0;

	ssize_t size = write(fd, buf, len);
	close(fd);

	if (size != len) return 0;
	return len;
}


/*
 * Recursively creates directories on `path`.
 * Returns 1 if somehow couldn't create one.
 */
int file_mkdir_p(const char *path) {
	int status = 0;
	char *tmp = strndup(path, 256);
	if (!tmp) return -2; // TODO: assign useful value

	size_t len = strlen(tmp);

	if (tmp[len - 1] == '/')
		tmp[len - 1] = '\0';

	char *p = NULL;
	for (p = tmp; *p != '\0'; p++) {
		if (*p == '/') {
			*p = '\0';
			status = mkdir(tmp, S_IRWXU | S_IRWXG);
			*p = '/';
		}
	}
	status = mkdir(tmp, S_IRWXU | S_IRWXG);
	free(tmp);
	return status;
}

