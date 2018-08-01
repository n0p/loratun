#include "modem.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#define CONFIG(n, v) { Config c={n, v}; listNodeAdd(param, (Config *)&c); }

int main() {

	List *param=listNew();

	void sig_handler(int signo) {

		printf("\n");

		loratun_modem_destroy();

		{
			Node *n=param->first;
			while (n) {
				listNodeDel(param, n);
				n=n->sig;
			}
			printf("freed\n");
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

	CONFIG("SerialPort", "/dev/ttyACM0");
	CONFIG("AT+APPKEY", "ce:e3:fa:63:e5:ee:e8:ff:28:dd:08:69:ca:48:87:1c\n");
	CONFIG("AT+NWSKEY", "cb:c9:a5:4e:de:35:4e:c9:33:61:cf:01:c3:71:e7:e2\n");
	CONFIG("AT+DADDR", "06:c6:f5:c0\n");
	CONFIG("AT+APPSKEY", "81:3f:2e:ad:c7:ec:ce:26:ae:b2:33:87:a9:b9:cb:45\n");
	CONFIG("AT+ADR", "0\n");
	CONFIG("AT+DR", "2\n");
	CONFIG("AT+DCS", "0\n");
	CONFIG("AT+TXP", "3\n");
	CONFIG("AT+CLASS", "C\n");
	CONFIG("AT+NJM", "1\n");

	/*
	{
		Node *n=param->first;
		while (n) {
			Config *c=(Config *)n->e;
			printf("key=%s value=%s\n", c->key, c->value);
			if (strcmp(c->key, "key2")==0) printf("*** parámetro me interesa!\n");
			n=n->sig;
		}
	}
	*/

	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	loratun_modem_init(param);
	loratun_modem_check_joined();
	
	sig_handler(SIGINT);

	return 0;

}
