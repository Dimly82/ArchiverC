CC = gcc

all: clean archiver unarchiver

archiver: main.c archiver.c archiver.h
	$(CC) $^ -o $@

unarchiver: main.c archiver.c archiver.h
	$(CC) $^ -DUNARCH -o $@

clean:
	rm -rf archiver unarchiver