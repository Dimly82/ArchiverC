CC = gcc

all: clean archiver unarchiver

archiver: main.c archiver.c archiver.h
	$(CC) $^ -o $@

unarchiver: main.c archiver.c archiver.h
	$(CC) $^ -DUNARCH -o $@

format:
	clang-format --verbose -i *.c *.h

clean:
	rm -rf archiver unarchiver