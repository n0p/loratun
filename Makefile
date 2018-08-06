CC = gcc
#CFLAGS = -g -O2 -Wall -D_GNU_SOURCE
#LDFLAGS = -lpthread -pthread

all: loratun udp6test

udp6test:
	$(CC) $(CFLAGS) $(LDFLAGS) udp6test.c -o udp6test

loratun:
	$(CC) $(CFLAGS) $(LDFLAGS) modemfoo/modem.c lib/list.c schc/context.c schc/schc.c loratun.c -o loratun

modemfoo:
	$(CC) $(CFLAGS) $(LDFLAGS) modemfoo/modem.c -o loratun

test: modemfoo loratun udp6test
	./loratun -u -d -i loratun -m "key=sample value" &
	@sleep 0.3
	ip addr add fd73:ab44:93dd:6b18::/64 dev loratun
	#@ip addr add fe80:0000:0000:0000::/48 dev loratun
	ip link set loratun up
	@sleep 0.1
	@killall -TSTP loratun
	@./udp6test
	#@./udp6test
	@killall -CONT loratun
	@sleep 0.1
	@killall -q loratun

lora: loratun udp6test
	./loratun -u -d -i loratun \
	-m SerialPort=/dev/ttyACM0 \
	-m AT+APPKEY=ce:e3:fa:63:e5:ee:e8:ff:28:dd:08:69:ca:48:87:1c \
	-m AT+NWSKEY=cb:c9:a5:4e:de:35:4e:c9:33:61:cf:01:c3:71:e7:e2 \
	-m AT+DADDR=06:c6:f5:c0 \
	-m AT+APPSKEY=81:3f:2e:ad:c7:ec:ce:26:ae:b2:33:87:a9:b9:cb:45 \
	-m AT+ADR=0 \
	-m AT+DR=2 \
	-m AT+DCS=0 \
	-m AT+TXP=3 \
	-m AT+CLASS=C \
	-m AT+NJM=1 \
	&
	@sleep 0.3
	ip addr add fd73:ab44:93dd:6b18::/64 dev loratun
	@#ip addr add fe80:0000:0000:0000::/48 dev loratun
	ip link set loratun up
	@sleep 0.1
	@killall -TSTP loratun
	./udp6test
	@sleep 0.2
	./udp6test
	@killall -CONT loratun
	@sleep 0.1
	@killall -q loratun

clean:
	@rm -f *.o loratun udp6test
