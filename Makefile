CC = gcc
CFLAGS = -g -O2 -Wall -D_GNU_SOURCE
#LDFLAGS = -lpthread -pthread

all: loratun udp6test

loratun: schc/context schc/schc

test:
	gcc udp6test.c -o udp6test && gcc schc/context.c schc/schc.c loratun.c -o loratun && ./loratun -u -d -i loratun &
	@sleep 0.3
	ip addr add fd73:ab44:93dd:6b18::/64 dev loratun
	@#ip addr add fe80:0000:0000:0000::/48 dev loratun
	ip link set loratun up
	@sleep 0.1
	@killall -TSTP loratun
	./udp6test
	@killall -CONT loratun
	@sleep 0.1
	@killall -q loratun

clean:
	@rm -f *.o loratun udp6test
