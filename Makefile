CFLAGS=-c -std=c99 -Wall -O3 -D_POSIX_C_SOURCE=199309L

all: min_demo

min_demo: main.o
	$(CC) main.o -o min_demo

main.o: main.c
	$(CC) $(CFLAGS) main.c

clean:
	rm -rf *.o min_demo

