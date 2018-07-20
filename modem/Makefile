CC = gcc
CFLAGS = -g -O2 -Wall -D_GNU_SOURCE
#LDFLAGS = -lpthread -pthread

all:
	$(CC) ../lib/list.c ../lib/serial.c modem.c test.c -o test

test: all
	./test

clean:
	-rm -f test *~
