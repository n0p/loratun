#ifndef __MODEM_H
#define __MODEM_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "modem_config.h"

extern pthread_mutex_t tun_mutex;
extern void con(char *msg, ...);
extern void err(char *msg, ...);
int modemtun_modem(List *modem_config);
extern int modemtun_modem_recv(uint8_t *data, int len); // callback
int modemtun_modem_send(uint8_t *data, int len);
int modemtun_modem_destroy();

#endif
