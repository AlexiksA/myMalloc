.PHONY: clean

CFLAGS := -Wall -Wextra

all: main.c
	gcc $(CFLAGS) $^ -o malloc

clean:
	rm -f malloc