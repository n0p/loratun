CC = gcc
CFLAGS = -std=c11 -D_GNU_SOURCE -g -O2 -Wall -Wno-pointer-sign
LDFLAGS = -lpthread -pthread
LORATUN = lib/list.c lib/modem_config.c schc/context.c schc/schc.c schc/schc_fields.c loratun.c -o loratun

all: loratun_modemfoo udp6test

udp6test:
	$(CC) $(CFLAGS) $(LDFLAGS) udp6test.c -o udp6test

loratun_modemfoo:
	$(CC) $(CFLAGS) $(LDFLAGS) modemfoo/modem.c $(LORATUN)

loratun_lorawan:
	$(CC) $(CFLAGS) $(LDFLAGS) lib/serial.c lorawan/modem.c $(LORATUN)

test_modemfoo: loratun_modemfoo udp6test
	bash -c "sleep 0.5 && ip addr add fd73:ab44:93dd:6b18::/64 dev loratun && ip link set loratun up && ./udp6test" &
	./loratun -u -d 2 -i loratun -m 'key=sample value'

test_lorawan: loratun_lorawan udp6test
	bash -c "sleep 0.5 && ip addr add fd73:ab44:93dd:6b18::/64 dev loratun && ip link set loratun up && ./udp6test" &
	./loratun -u -d 2 -i loratun \
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
	@rm -f *.o loratun udp6test
