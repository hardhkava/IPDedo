#ifndef RESOLVER_H
#define RESOLVER_H

#include "structs.h"

/*entry point for thread created by main*/
void* handleClient(void* arg);

/*return 1 if found and fill res, else 0*/
int resolve(struct DNSCache* cache, const char* domain, char* res);

#endif
