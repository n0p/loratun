CC = gcc
CFLAGS = -g -O0 -ggdb -Wall -D_GNU_SOURCE -Wno-pointer-sign
LDFLAGS = -lpthread -pthread

all:
	$(CC) $(CFLAGS) $(LDFLAGS) ../lib/list.c ../lib/modem_config.c ../lib/serial.c modem.c test.c -o test

test2:
	$(CC) $(CFLAGS) $(LDFLAGS) ../lib/list.c ../lib/modem_config.c ../lib/serial.c modem2.c test.c -o test
	/bin/stty -F /dev/ttyACM0 9600 cs8 -cstopb -parenb ignbrk -brkint -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
	./test

test: all
	./test

clean:
	-rm -f test *~
