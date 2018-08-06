#ifndef __MODEM_H
#define __MODEM_H

#include <stdio.h>

#include "list.h"
//#include "serial.h"
//#include "debug.h"

struct modem_config {
	char key[512];
	char value[512];
};
typedef struct modem_config ModemConfig;

int loratun_modem(List *modem_config);
extern int loratun_modem_recv(char *data, int len); // callback
int loratun_modem_send(char *data, int len);
int loratun_modem_destroy();

#endif
