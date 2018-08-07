#include "modem_config.h"

List *modem_config;

// modem config list creation
void modem_config_init() {
	modem_config=listNew();
}

// modem config add item
void modem_config_add(char *k, char *v) {
	ModemConfig *c=malloc(sizeof(ModemConfig));
	strcpy(c->key, k);
	strcpy(c->value, v);
	listNodeAdd(modem_config, (ModemConfig *)c);
}

// modem config delete
void modem_config_del(ModemConfig *c) {
	free(c);
}

// modem config get value by key name
char *modem_config_get(char *key) {
	Node *n=modem_config->first;
	while (n) {
		ModemConfig *c=(ModemConfig *)n->e;
		if (!strcmp(c->key, key)) return c->value;
		n=n->sig;
	}
}

// modem config print list
void modem_config_print() {
	Node *n=modem_config->first;
	while (n) {
		ModemConfig *c=(ModemConfig *)n->e;
		printf("%s=%s\n", c->key, c->value);
		n=n->sig;
	}
}

// modem config entire list free
void modem_config_destroy() {
	Node *n=modem_config->first;
	while (n) {
		modem_config_del((ModemConfig *)n->e);
		listNodeDel(modem_config, n);
		n=n->sig;
	}
	listFree(modem_config);
}
