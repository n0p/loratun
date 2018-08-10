#ifndef __CONFIG_H
#define __CONFIG_H

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

struct config {
	char key[512];
	char value[512];
};
typedef struct config Config;

List *config;

void config_init();
void config_add(char *k, char *v);
void config_del(Config *c);
char *config_get(char *key);
void config_print();
void config_destroy();

#endif
