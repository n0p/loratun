#ifndef __ISEND_H
#define __ISEND_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int isend_init(uint8_t *dst_addr, uint16_t dst_port);
void isend_begin();
void isend_add(uint8_t *data, uint16_t len);
void isend_commit();
void isend_destroy();

#endif
