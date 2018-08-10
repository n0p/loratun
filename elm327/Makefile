CC = gcc
CFLAGS = -g -O0 -ggdb -Wall -D_GNU_SOURCE -Wno-pointer-sign
LDFLAGS = -lpthread -pthread

all:
	$(CC) $(CFLAGS) $(LDFLAGS) ../lib/config.c ../lib/list.c ../lib/serial.c elm327.c -o elm327

test: all
	./elm327

clean:
	-rm -f elm327 *~
