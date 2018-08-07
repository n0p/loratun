#ifndef __SCHC_FIELDS_H
#define __SCHC_FIELDS_H

#include "schc.h"

int buffer2fields(unsigned char *buffer, int *len, struct field_values *p);
int fields2buffer(struct field_values *p, char *buffer, int *len);

#endif
