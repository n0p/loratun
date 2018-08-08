CC = gcc
CFLAGS = -g -O0 -ggdb -Wall -D_GNU_SOURCE
#LDFLAGS = -lpthread -pthread

all:
	$(CC) $(CFLAGS) ../lib/list.c ../lib/serial.c modem.c test.c -o test

test: all
	./test

clean:
	-rm -f test *~
