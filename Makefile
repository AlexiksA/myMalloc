CC = gcc
CFLAGS := -Wall -Wextra -std=c11

.PHONY: clean

myMal.o: myMal.c
	$(CC) $(CFLAGS) -c $^ -o myMal.o

all: main.c myMal.o
	gcc $(CFLAGS) $^ -o malloc

clean:
	rm -f malloc