#include "archiver.h"

void archive(const char *arch_name, const char *dir_path) {
  int archive_fd = open(arch_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (archive_fd == -1) {
    perror("open archive");
    return;
  }

  archive_directory(dir_path, archive_fd);
  close(archive_fd);
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

    char *buffer = malloc(st.st_size);
    if (read(src_fd, buffer, st.st_size) != st.st_size) {
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

void unarchive(const char *arch_path) {
  int archive_fd = open(arch_path, O_RDONLY);
  if (archive_fd == -1) {
    perror("open archive");
    return;
  }
  unarchive_directory(archive_fd);
  close(archive_fd);
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
    char command[PATH_MAX + 10] = "";
    sprintf(command, "mkdir -p %s", file_name);
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
    size_t decompressed_size = rle_decompress(compressed_data, compressed_size, &decompressed_data);
    free(compressed_data);

    write(fd, decompressed_data, decompressed_size);
    free(decompressed_data);
    close(fd);
  }
}

void unarchive_directory(int archive_fd) {
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
    while (i + run_len < input_size && input[i + run_len] == current_ch && run_len < 255)
      run_len++;

    (*output)[out_index++] = (char) run_len;
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
