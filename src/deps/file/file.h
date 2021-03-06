
//
// utils.h
//
// Copyright (c) 2013 TJ Holowaychuk <tj@vision-media.ca>
//

#ifndef FILE_H
#define FILE_H

#include <sys/stat.h>
#include <sys/types.h>

off_t file_size(const char *filename);

int file_exists(const char *filename);

char *file_read(const char *filename);

size_t file_write(const char *filename, char *buf, int flags, int mode);

int file_mkdir_p(const char *path);

#endif
