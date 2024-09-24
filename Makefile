CC = gcc
CFLAGS = -Wall -Werror -Wextra

all: clean archiver unarchiver

archiver: main.c archiver.c archiver.h
	$(CC) $(CFLAGS) $^ -o $@

unarchiver: main.c archiver.c archiver.h
	$(CC) $(CFLAGS) $^ -DUNARCH -o $@

clean:
	rm -rf archiver unarchiver