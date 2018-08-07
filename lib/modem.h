#ifndef __MODEM_H
#define __MODEM_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "modem_config.h"

extern pthread_mutex_t tun_mutex;
extern void con(char *msg, ...);
int loratun_modem(List *modem_config);
extern int loratun_modem_recv(char *data, int len); // callback
int loratun_modem_send(char *data, int len);
int loratun_modem_destroy();

#endif
