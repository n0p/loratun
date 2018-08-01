#ifndef __MODEM_H
#define __MODEM_H

#include <stdio.h>

#include "../lib/list.h"
#include "../lib/serial.h"
//#include "../lib/debug.h"

#define MAX_LINE 1024

struct config {
	char key[512];
	char value[512];
};
typedef struct config Config;

int loratun_modem(List *param);
extern int loratun_modem_recv(char *data, int len); // callback
int loratun_modem_send(char *data, int len);
int loratun_modem_destroy();

#endif
