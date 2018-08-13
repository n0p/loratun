#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/modem.h"

List *modem_config;

int loratun_modem_recv(uint8_t *data, int len) {
	printf("test.c: we got a callback with %d bytes\n Contents: %s\n", len, data);
	return 0;
}

int main() {

	modem_config_init();

	void sig_handler(int signo) {
		printf("\n");
		loratun_modem_destroy();
		modem_config_destroy();
		exit(0);
	}

	modem_config_add("SerialPort", "/dev/ttyACM0");
	 // App keys for pleiades gateway
	modem_config_add("AT+APPKEY", "ce:e3:fa:63:e5:ee:e8:ff:28:dd:08:69:ca:48:87:1c");
	modem_config_add("AT+NWSKEY", "cb:c9:a5:4e:de:35:4e:c9:33:61:cf:01:c3:71:e7:e2");
	modem_config_add("AT+DADDR", "06:c6:f5:c0");
	modem_config_add("AT+APPSKEY", "81:3f:2e:ad:c7:ec:ce:26:ae:b2:33:87:a9:b9:cb:45");
	
	/*
	// App keys for TTN test app
	modem_config_add("AT+APPKEY", "35:4D:D6:24:10:56:7E:3C:8F:97:75:2C:EA:F4:05:F1");
	modem_config_add("AT+APPEUI", "70:B3:D5:7E:D0:00:99:FB");
	*/
	
	modem_config_add("AT+ADR", "0");
	modem_config_add("AT+DR", "7");
	modem_config_add("AT+DCS", "0");
	modem_config_add("AT+TXP", "3");
	modem_config_add("AT+CLASS", "C");
	modem_config_add("AT+NJM", "1");

	uint8_t testpkt [] = {0x07, 0x40, 0xD4, 0x6C, 0x48, 0x4F, 0x4C, 0x40, 0x21, 0x00};
	
	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	loratun_modem(modem_config);
	loratun_modem_send(testpkt, sizeof(testpkt)); // TODO: we cannot call this function since we are locked in a loop
	
	sig_handler(SIGINT);

	return 0;

}
