#include "archiver.h"

char *get_base_path(const char *full_path) {
  char *base_path = malloc(sizeof(char) * PATH_MAX);
  char *prev_token = malloc(sizeof(char) * PATH_MAX);
  char *full_path_cp = malloc(sizeof(char) * strlen(full_path) + 1);

  strcpy(full_path_cp, full_path);
  char *token = strtok(full_path_cp, "/");
  strcpy(prev_token, token);
  while (token) {
    token = strtok(NULL, "/");
    if (token)
      strcpy(prev_token, token);
  }

  strcpy(base_path, prev_token);

  free(full_path_cp);
  free(prev_token);
  return base_path;
}

void archive(const char *arch_name, const char *dir_path, const char *def_dir) {
  char *full_path = malloc(sizeof(char) * PATH_MAX);
  char *command = malloc(sizeof(char) * PATH_MAX + 8);
  sprintf(full_path, "%s/%s", def_dir, arch_name);
  sprintf(command, "mkdir -p \"%s\"", def_dir);
  if (system(command) != 0) {
    perror("mkdir");
    return;
  }

  char *base_path = get_base_path(dir_path);

  int archive_fd =
      open(full_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (archive_fd == -1) {
    perror("open archive");
    return;
  }

  struct stat64 st;
  if (stat64(dir_path, &st) == -1) {
    perror("stat64");
    return;
  }

  size_t name_len = strlen(base_path);
  char type = 'D';
  write(archive_fd, &name_len, sizeof(size_t));
  write(archive_fd, &type, sizeof(char));
  write(archive_fd, base_path, name_len);
  write(archive_fd, &st.st_size, sizeof(off64_t));

  archive_directory(dir_path, base_path, archive_fd);
  free(full_path);
  free(command);
  free(base_path);
  close(archive_fd);
}

void archive_file(const char *file_path, const char *base_dir, int archive_fd) {
  struct stat64 st;
  if (stat64(file_path, &st) == -1) {
    perror("stat64");
    return;
  }

  char *new_path;
  new_path = strstr(file_path, base_dir);
  size_t name_len = strlen(new_path) + 1;
  write(archive_fd, &name_len, sizeof(size_t));
  char type = S_ISDIR(st.st_mode) ? 'D' : 'F';
  write(archive_fd, &type, sizeof(char));
  write(archive_fd, new_path, name_len);
  write(archive_fd, &st.st_size, sizeof(off64_t));

  if (S_ISREG(st.st_mode)) {
    int src_fd = open(file_path, O_RDONLY);
    if (src_fd == -1) {
      perror("open");
      return;
    }

    char *buffer = malloc(st.st_size);
    if ((read(src_fd, buffer, st.st_size)) != st.st_size) {
      perror("read");
      free(buffer);
      close(src_fd);
      return;
    }
    close(src_fd);

    char *compressed_data;
    size_t compressed_size = rle_compress(buffer, st.st_size, &compressed_data);
    free(buffer);

    write(archive_fd, &compressed_size, sizeof(size_t));
    write(archive_fd, compressed_data, compressed_size);
    free(compressed_data);
  }
}

void archive_directory(const char *directory_path, const char *base_dir,
                       int archive_fd) {
  int fd = open(directory_path, O_RDONLY | O_DIRECTORY);
  if (fd == -1) {
    perror("open");
    return;
  }

  char buffer[BUF_SIZE];
  ssize_t nread;
  dir *d;
  int bpos;

  while ((nread = syscall(SYS_getdents, fd, buffer, BUF_SIZE)) > 0) {
    for (bpos = 0; bpos < nread;) {
      d = (dir *)(buffer + bpos);
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
        archive_file(full_path, base_dir, archive_fd);
        archive_directory(full_path, base_dir, archive_fd);
      }
      bpos += d->d_reclen;
    }
  }
  if (nread == -1)
    perror("getdents");

  lseek(fd, 0, SEEK_SET);

  while ((nread = syscall(SYS_getdents, fd, buffer, BUF_SIZE)) > 0) {
    for (bpos = 0; bpos < nread;) {
      d = (dir *)(buffer + bpos);
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

      if (S_ISREG(st.st_mode))
        archive_file(full_path, base_dir, archive_fd);
      bpos += d->d_reclen;
    }
  }
  if (nread == -1)
    perror("getdents");
  close(fd);
}

void unarchive(const char *arch_path, const char *dir_path) {
  char *command = malloc(sizeof(char) * PATH_MAX + 8);
  sprintf(command, "mkdir -p \"%s\"", dir_path);
  if (system(command) != 0) {
    perror("mkdir");
    return;
  }

  int archive_fd = open(arch_path, O_RDONLY);
  if (archive_fd == -1) {
    perror("open archive");
    return;
  }
  unarchive_directory(archive_fd, dir_path);
  free(command);
  close(archive_fd);
}

void unarchive_file(int archive_fd, const char *dir_path) {
  size_t name_len;
  char file_name[PATH_MAX];
  char type;
  off64_t file_size;

  read(archive_fd, &name_len, sizeof(size_t));
  read(archive_fd, &type, sizeof(char));
  read(archive_fd, file_name, name_len);
  read(archive_fd, &file_size, sizeof(off64_t));

  char *full_path =
      malloc(sizeof(char) * (strlen(file_name) + strlen(dir_path)) + 1);
  sprintf(full_path, "%s/%s", dir_path, file_name);

  if (type == 'D') {
    char command[PATH_MAX + 10] = "";
    sprintf(command, "mkdir -p \"%s\"", full_path);
    if (system(command) != 0) {
      perror("mkdir");
      return;
    }
    unarchive_directory(archive_fd, dir_path);
  } else if (type == 'F') {
    int fd = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      perror("open");
      return;
    }

    size_t compressed_size;
    read(archive_fd, &compressed_size, sizeof(size_t));

    char *compressed_data = malloc(compressed_size);
    if (read(archive_fd, compressed_data, compressed_size) != compressed_size) {
      perror("read");
      free(compressed_data);
      close(fd);
      return;
    }

    char *decompressed_data;
    size_t decompressed_size =
        rle_decompress(compressed_data, compressed_size, &decompressed_data);
    free(compressed_data);

    write(fd, decompressed_data, decompressed_size);
    free(decompressed_data);
    close(fd);
  }
  free(full_path);
}

void unarchive_directory(int archive_fd, const char *dir_path) {
  while (1) {
    size_t name_len;
    char type;

    off64_t curr_pos = lseek64(archive_fd, 0, SEEK_CUR);
    ssize_t bytes_read = read(archive_fd, &name_len, sizeof(size_t));
    if (bytes_read == 0)
      return;
    if (bytes_read == -1) {
      perror("read");
      return;
    }

    read(archive_fd, &type, sizeof(char));
    lseek64(archive_fd, curr_pos, SEEK_SET);
    if (type == 'D' || type == 'F')
      unarchive_file(archive_fd, dir_path);
    else
      return;
  }
}

size_t rle_compress(const char *input, size_t input_size, char **output) {
  *output = malloc(2 * input_size);
  if (!(*output)) {
    perror("malloc");
    exit(1);
  }

  size_t out_index = 0;
  for (size_t i = 0; i < input_size;) {
    char current_ch = input[i];
    size_t run_len = 1;
    while (i + run_len < input_size && input[i + run_len] == current_ch &&
           run_len < 255)
      run_len++;

    (*output)[out_index++] = (char)run_len;
    (*output)[out_index++] = current_ch;

    i += run_len;
  }

  return out_index;
}

size_t rle_decompress(const char *input, size_t input_size, char **output) {
  *output = malloc(input_size * 255);
  if (*output == NULL) {
    perror("malloc");
    exit(1);
  }

  size_t out_index = 0;
  for (size_t i = 0; i < input_size; i += 2) {
    char run_length = input[i];
    char current_char = input[i + 1];

    for (int j = 0; j < run_length; j++) {
      (*output)[out_index++] = current_char;
    }
  }

  return out_index;
}
