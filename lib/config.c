#include "config.h"

// config list creation
void config_init() {
	config=listNew();
}

// config add item
void config_add(char *k, char *v) {
	Config *c=malloc(sizeof(Config));
	strcpy(c->key, k);
	strcpy(c->value, v);
	listNodeAdd(config, (Config *)c);
}

// config delete
void config_del(Config *c) {
	free(c);
}

// config get value by key name
char *config_get(char *key) {
	Node *n=config->first;
	while (n) {
		Config *c=(Config *)n->e;
		if (!strcmp(c->key, key)) return c->value;
		n=n->next;
	}
	return NULL;
}

// config print list
void config_print() {
	Node *n=config->first;
	while (n) {
		Config *c=(Config *)n->e;
		printf("%s=%s\n", c->key, c->value);
		n=n->next;
	}
}

// config entire list free
void config_destroy() {
	Node *n=config->first;
	while (n) {
		config_del((Config *)n->e);
		listNodeDel(config, n);
		n=n->next;
	}
	listFree(config);
}
