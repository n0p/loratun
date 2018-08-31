CC = gcc
CFLAGS = -std=c11 -D_GNU_SOURCE -g -O2 -Wall -Wno-pointer-sign
LDFLAGS = -lpthread -pthread
MODEMTUN = lib/list.c lib/modem_config.c schc/context.c schc/schc.c schc/schc_fields.c modemtun.c -o

all: footun loratun udp6test

udp6test:
	$(CC) $(CFLAGS) $(LDFLAGS) udp6test.c -o udp6test

footun:
	$(CC) $(CFLAGS) $(LDFLAGS) modemfoo/modem.c $(MODEMTUN) footun

loratun:
	$(CC) $(CFLAGS) $(LDFLAGS) lib/serial.c lorawan/modem.c $(MODEMTUN) loratun

test_footun: footun udp6test
	bash -c "sleep 0.5 && ip addr add fd73:ab44:93dd:6b18::/64 dev footun && ip link set footun up && ./udp6test" &
	./footun -d 2 -i footun -m 'key=sample value'

test_loratun: loratun udp6test
	bash -c "sleep 0.5 && ip addr add fd73:ab44:93dd:6b18::/64 dev loratun && ip link set loratun up && ./udp6test" &
	./loratun -d 2 -i loratun \
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

clean:
	@rm -f *.o footun loratun udp6test
