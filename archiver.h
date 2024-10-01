#ifndef ARCHIVER_ARCHIVER_H
#define ARCHIVER_ARCHIVER_H

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define D_TYPE_DIR 4

#define BLACK $(tput setaf 0)
#define RED $(tput setaf 1)
#define GREEN $(tput setaf 2)
#define YELLOW $(tput setaf 3)
#define LIME_YELLOW $(tput setaf 190)
#define POWDER_BLUE $(tput setaf 153)
#define BLUE $(tput setaf 4)
#define MAGENTA $(tput setaf 5)
#define CYAN $(tput setaf 6)
#define WHITE $(tput setaf 7)
#define BRIGHT $(tput bold)
#define NORMAL $(tput sgr0)
#define BLINK $(tput blink)
#define REVERSE $(tput smso)
#define UNDERLINE $(tput smul)

typedef struct {
  long d_ino;
  long d_off;
  unsigned short d_reclen;
  char d_name[];
} dir;

char *get_base_path(const char *full_path);

void archive(const char *arch_name, const char *dir_path, const char *def_dir);

int archive_file(const char *file_path, const char *base_dir, int archive_fd);

int archive_directory(const char *directory_path, const char *base_dir,
                      int archive_fd);

void unarchive(const char *arch_path, const char *dir_path);

int unarchive_file(int archive_fd, const char *dir_path);

int unarchive_directory(int archive_fd, const char *dir_path);

size_t rle_compress(const char *input, size_t input_size, char **output);

size_t rle_decompress(const char *input, size_t input_size, char **output);

#endif // ARCHIVER_ARCHIVER_H
