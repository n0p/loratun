CC = gcc
CFLAGS = -g -O2 -Wall -D_GNU_SOURCE
LDFLAGS = -lpthread -pthread

all:
	$(CC) $(CFLAGS) $(LDFLAGS) ../lib/modem_config.c ../lib/list.c ../lib/serial.c modem.c test.c -o test

test: all
	./test

clean:
	-rm -f test *~
