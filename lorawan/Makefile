CC = gcc
CFLAGS = -g -O0 -ggdb -Wall -D_GNU_SOURCE -Wno-pointer-sign
LDFLAGS = -lpthread -pthread

all:
	$(CC) $(CFLAGS) $(LDFLAGS) ../lib/list.c ../lib/modem_config.c ../lib/serial.c modem.c test.c -o test

test: all
	./test

clean:
	-rm -f test *~
