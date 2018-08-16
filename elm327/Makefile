CC = gcc
CFLAGS = -g -O0 -ggdb -Wall -D_GNU_SOURCE -Wno-pointer-sign
LDFLAGS = -lm -lgps
# -lpthread -pthread

all:
	$(CC) $(CFLAGS) ../lib/config.c ../lib/list.c ../lib/serial.c isend.c elm327.c $(LDFLAGS) -o elm327

test: all
	./elm327

clean:
	-rm -f elm327 *~
