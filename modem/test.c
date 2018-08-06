#include "modem.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int loratun_modem_recv(char *data, int len)
{
	printf("test.c: we got a callback with %d bytes\n Contents: %s\n", len, data);
	return 0;
}

int main() {

	List *param=listNew();

	void configAdd(char *k, char *v) {
		Config *c=malloc(sizeof(Config));
		strcpy(c->key, k);
		strcpy(c->value, v);
		listNodeAdd(param, (Config *)c);
	}

	void configDel(Config *c) {
		free(c);
	}

	void configList() {
		Node *n=param->first;
		while (n) {
			Config *c=(Config *)n->e;
			printf("%s=%s\n", c->key, c->value);
			//if (strcmp(c->key, "key2")==0) printf("*** parámetro me interesa!\n");
			n=n->sig;
		}
	}

	void sig_handler(int signo) {

		printf("\n");

		loratun_modem_destroy();

		{
			Node *n=param->first;
			while (n) {
				configDel((Config *)n->e);
				listNodeDel(param, n);
				n=n->sig;
			}
		}
		listFree(param);

		exit(0);

	}

	/*
	 * AT+APPKEY=ce:e3:fa:63:e5:ee:e8:ff:28:dd:08:69:ca:48:87:1c
	 * AT+APPEUI=00:00:00:00:00:00:00:02
	 * AT+NWKSKEY=cb:c9:a5:4e:de:35:4e:c9:33:61:cf:01:c3:71:e7:e2
	 * AT+DADDR=06:c6:f5:c0
	 * AT+APPSKEY=81:3f:2e:ad:c7:ec:ce:26:ae:b2:33:87:a9:b9:cb:45
	 * AT+NWKID=03:02:01:00
	 * AT+DEUI=31:31:35:38:56:37:89:18
	 * AT+ADR=0
	 * AT+DR=2
	 * AT+DCS=0
	 * AT+TXP=3
	 * AT+CLASS=C
	 * AT+NJM=1
	 * AT+JOIN 
	 */

	configAdd("SerialPort", "/dev/ttyACM0");
	configAdd("AT+APPKEY", "ce:e3:fa:63:e5:ee:e8:ff:28:dd:08:69:ca:48:87:1c");
	configAdd("AT+NWSKEY", "cb:c9:a5:4e:de:35:4e:c9:33:61:cf:01:c3:71:e7:e2");
	configAdd("AT+DADDR", "06:c6:f5:c0");
	configAdd("AT+APPSKEY", "81:3f:2e:ad:c7:ec:ce:26:ae:b2:33:87:a9:b9:cb:45");
	configAdd("AT+ADR", "0");
	configAdd("AT+DR", "7");
	configAdd("AT+DCS", "0");
	configAdd("AT+TXP", "3");
	configAdd("AT+CLASS", "C");
	configAdd("AT+NJM", "1");
	configList();

	char testpkt [] = {0x07, 0x40, 0xD4, 0x6C, 0x48, 0x4F, 0x4C, 0x40, 0x21, 0x00};
	
	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	loratun_modem(param);
	loratun_modem_send(testpkt, sizeof(testpkt)); // TODO: we cannot call this function since we are locked in a loop
	
	sig_handler(SIGINT);

	return 0;

}
