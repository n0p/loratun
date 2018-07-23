#include "modem.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

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
		}
		listFree(param);

		exit(0);

	}

	Config configuracion={"key","value"};
	Config configuracion_buena={"key2","value"};

	listNodeAdd(param, (Config *)&configuracion);
	listNodeAdd(param, (Config *)&configuracion_buena);
	listNodeAdd(param, (Config *)&configuracion);
	listNodeAdd(param, (Config *)&configuracion);
	listNodeAdd(param, (Config *)&configuracion);
	listNodeAdd(param, (Config *)&configuracion);
	listNodeAdd(param, (Config *)&configuracion);

	{
		Node *n=param->first;
		while (n) {
			Config *c=(Config *)n->e;
			printf("key=%s value=%s\n", c->key, c->value);
			if (strcmp(c->key, "key2")==0) printf("*** parámetro me interesa!\n");
			n=n->sig;
		}
	}

	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	loratun_modem_init(param);

	sig_handler(SIGINT);

	return 0;

}
