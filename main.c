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
  archive(argv[1], argv[2]);
#endif

#ifdef UNARCH
  unarchive(argv[1]);
#endif

  return 0;
}