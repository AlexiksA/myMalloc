CC = gcc
CFLAGS := -Wall -Wextra -std=c11

.PHONY: all clean

all: main.c myMal.o
	$(CC) $(CFLAGS) $^ -o malloc

myMal.o: myMal.c myMal.h
	$(CC) $(CFLAGS) -c $< -o myMal.o

clean:
	rm -f malloc myMal.o
