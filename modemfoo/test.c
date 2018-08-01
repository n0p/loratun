#include "modem.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

List *param;

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

int loratun_modem_recv(char *data, int len) {
	printf("*** He recibido len(%d)=%s\n", len, data);
}

int *modem_thread(void *p)
{

	/* increment x to 100 */
	//int *x_ptr = (int *)p;
	//while(++(*x_ptr) < 100);

	loratun_modem(param);

	printf("modem finished\n");

	/* the function must return something - NULL will do */
	return NULL;

}

int main() {

	param=listNew();

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

	configAdd("SerialPort", "/dev/ttyACM0");
	configAdd("AT+APPKEY", "ce:e3:fa:63:e5:ee:e8:ff:28:dd:08:69:ca:48:87:1c");
	configAdd("AT+NWSKEY", "cb:c9:a5:4e:de:35:4e:c9:33:61:cf:01:c3:71:e7:e2");
	configAdd("AT+DADDR", "06:c6:f5:c0");
	configAdd("AT+APPSKEY", "81:3f:2e:ad:c7:ec:ce:26:ae:b2:33:87:a9:b9:cb:45");
	configAdd("AT+ADR", "0");
	configAdd("AT+DR", "2");
	configAdd("AT+DCS", "0");
	configAdd("AT+TXP", "3");
	configAdd("AT+CLASS", "C");
	configAdd("AT+NJM", "1");
	configList();

	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");


	// thread
	pthread_t modem_thread_t;

	// start thread
	if (pthread_create(&modem_thread_t, NULL, modem_thread, 0)) {
		perror("Error creating thread");
		return 1;
	}

	// padre
	int i=0;
	for (i=0;i<3;i++) {
		sleep(1);
		printf("****** Send %d\n", i);
		loratun_modem_send("ABC\0", 4);
	}

	/// wait for the thread to finish
	if (pthread_join(modem_thread_t, NULL)) {
		perror("Error joining thread");
		return 2;
	}

	// terminar
	sig_handler(SIGINT);

	// fallback
	return 0;

}
