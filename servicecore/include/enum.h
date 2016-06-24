
#ifndef ENUM_LOOKUP_H
#define ENUM_LOOKUP_H

#include "sometools.h"
#define MAX_ENUM_LOOKUP_RESULTS 10

typedef struct enum_lookup_res{
	char *sip_address[MAX_ENUM_LOOKUP_RESULTS];
}enum_lookup_res_t;

bool_t is_enum(const char *sipaddress, char **enum_domain);
int enum_lookup(const char *enum_domain, enum_lookup_res_t **res);
void enum_lookup_res_free(enum_lookup_res_t *res);

#endif
