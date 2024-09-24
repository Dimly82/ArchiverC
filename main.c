#include "archiver.h"

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