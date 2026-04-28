#ifndef CACHE_H
#define CACHE_H

#include "structs.h"

void cacheInit(struct DNSCache* cache);

/*finds domain in cache, return 1 if hit and 0 if miss*/
int cacheLookup(struct DNSCache* cache, const char* domain, struct DNSRecord* result);

void cacheInsert(struct DNSCache* cache, const struct DNSRecord* record);

void cacheDestroy(struct DNSCache* cache);

/*marks a cache ebntry as invalid*/
void cacheInvalidate(struct DNSCache* cache, const char* domain);

#endif
