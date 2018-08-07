#ifndef __SCHC_FIELDS_H
#define __SCHC_FIELDS_H

#include "schc.h"

int buffer2fields(uint8_t *buffer, int *len, struct field_values *p);
int fields2buffer(struct field_values *p, uint8_t *buffer, int *len);

#endif
