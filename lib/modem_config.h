#ifndef __MODEM_CONFIG_H
#define __MODEM_CONFIG_H

#include <stdlib.h>
#include <string.h>

#include "list.h"

struct modem_config {
	char key[512];
	char value[512];
};
typedef struct modem_config ModemConfig;

extern List *modem_config;

void modem_config_init();
void modem_config_add(char *k, char *v);
void modem_config_del(ModemConfig *c);
char *modem_config_get(char *key);
void modem_config_print();
void modem_config_destroy();

#endif
