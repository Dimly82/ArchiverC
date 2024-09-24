#ifndef ARCHIVER_ARCHIVER_H
#define ARCHIVER_ARCHIVER_H

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/syscall.h>
#include <errno.h>

#define BUF_SIZE 1024
#define D_TYPE_DIR 4

typedef struct {
  long d_ino;
  long d_off;
  unsigned short d_reclen;
  char d_name[];
} dir;

void archive_file(const char *file_path, int archive_fd);

void archive_directory(const char *directory_path, int archive_fd);

void unarchive_file(int archive_fd);

void unarchive_directory(int archive_fd);

#endif //ARCHIVER_ARCHIVER_H
