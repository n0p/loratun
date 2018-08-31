CC = gcc
CFLAGS = -std=c11 -D_GNU_SOURCE -g -O2 -Wall -Wno-pointer-sign
LDFLAGS = -lm -lgps

all:
	$(CC) $(CFLAGS) ../lib/config.c ../lib/list.c ../lib/serial.c isend.c elm327app.c $(LDFLAGS) -o elm327app

test: all
	./elm327app

clean:
	-rm -f elm327app *~
