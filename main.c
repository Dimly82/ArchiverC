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

int main(int argc, char *argv[]) {
#ifndef UNARCH
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <archive> <directory> <default directory>\n", argv[0]);
    return 1;
  }
#endif

#ifdef UNARCH
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <archive>\n", argv[0]);
    return 1;
  }
#endif

#ifndef UNARCH
  int archive_fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
#endif

#ifdef UNARCH
  int archive_fd = open(argv[1], O_RDONLY);
#endif
  if (archive_fd == -1) {
    perror("open archive");
    return 1;
  }

#ifndef UNARCH
//  write(archive_fd, "#eto archiv", 11);
  archive_directory(argv[2], archive_fd);
#endif

#ifdef UNARCH
  unarchive_directory(archive_fd);
#endif

  close(archive_fd);
  return 0;
}

void archive_file(const char *file_path, int archive_fd) {
  struct stat st;
  if (stat(file_path, &st) == -1) {
    perror("stat");
    return;
  }

  size_t name_len = strlen(file_path) + 1;
  write(archive_fd, &name_len, sizeof(size_t));
  char type = S_ISDIR(st.st_mode) ? 'D' : 'F';
  write(archive_fd, &type, sizeof(char));
  write(archive_fd, file_path, name_len);
  write(archive_fd, &st.st_size, sizeof(off_t));

  if (S_ISREG(st.st_mode)) {
    int src_fd = open(file_path, O_RDONLY);
    if (src_fd == -1) {
      perror("open");
      return;
    }

    char buffer[BUF_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, buffer, BUF_SIZE)) > 0)
      write(archive_fd, buffer, bytes_read);

    close(src_fd);
  }
}

void archive_directory(const char *directory_path, int archive_fd) {
  int fd = open(directory_path, O_RDONLY | O_DIRECTORY);
  if (fd == -1) {
    perror("open");
    return;
  }

  char buffer[BUF_SIZE];
  int nread;
  dir *d;
  int bpos;

  while ((nread = syscall(SYS_getdents, fd, buffer, BUF_SIZE)) > 0) {
    for (bpos = 0; bpos < nread;) {
      d = (dir *) (buffer + bpos);
      char *entry_name = d->d_name;

      if (!strcmp(entry_name, ".") || !strcmp(entry_name, "..")) {
        bpos += d->d_reclen;
        continue;
      }

      char full_path[PATH_MAX];
      snprintf(full_path, PATH_MAX, "%s/%s", directory_path, entry_name);

      struct stat st;
      if (stat(full_path, &st) == -1) {
        perror("stat");
        bpos += d->d_reclen;
        continue;
      }

      if (S_ISDIR(st.st_mode)) {
        archive_file(full_path, archive_fd);
        archive_directory(full_path, archive_fd);
      } else if (S_ISREG(st.st_mode))
        archive_file(full_path, archive_fd);
      bpos += d->d_reclen;
    }
  }

  if (nread == -1)
    perror("getdents");
  close(fd);
}

void unarchive_file(int archive_fd) {
  size_t name_len;
  char file_name[PATH_MAX];
  char type;
  off_t file_size;

  read(archive_fd, &name_len, sizeof(size_t));
  read(archive_fd, &type, sizeof(char));
  read(archive_fd, file_name, name_len);
  read(archive_fd, &file_size, sizeof(off_t));

  if (type == 'D') {
    char command[256] = "";
    sprintf(command, "mkdir -p %s", file_name);
//    system(command);
//    if (mkdir(file_name, 0700) == -1 && errno != EEXIST) {
    if (system(command) == -1) {
      perror("mkdir");
      return;
    }
    unarchive_directory(archive_fd);
  } else if (type == 'F') {
    int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      perror("open");
      return;
    }

    char buffer[BUF_SIZE];
    ssize_t bytes_left = file_size;
    ssize_t bytes_read, bytes_written;

    while (bytes_left > 0) {
      bytes_read = read(archive_fd, buffer, (bytes_left > BUF_SIZE) ? BUF_SIZE : bytes_left);
      if (bytes_read == -1) {
        perror("read");
        close(fd);
        return;
      }

      bytes_written = write(fd, buffer, bytes_read);
      if (bytes_written == -1) {
        perror("write");
        close(fd);
        return;
      }

      bytes_left -= bytes_read;
    }
    close(fd);
  }
}

void unarchive_directory(int archive_fd) {
//  char test_str[11];
//  read(archive_fd, test_str, 11);
//  if (strcmp(test_str, "#eto archiv") != 0) {
//    write(2, "File is not an archive", 22);
//    return;
//  }

  while (1) {
    size_t name_len;
    char type;

    off_t curr_pos = lseek(archive_fd, 0, SEEK_CUR);
    ssize_t bytes_read = read(archive_fd, &name_len, sizeof(size_t));
    if (bytes_read == 0)
      return;
    if (bytes_read == -1) {
      perror("read");
      return;
    }

    read(archive_fd, &type, sizeof(char));
    lseek(archive_fd, curr_pos, SEEK_SET);
    if (type == 'D' || type == 'F')
      unarchive_file(archive_fd);
    else return;
  }
}