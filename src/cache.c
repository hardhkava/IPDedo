#include <stdio.h>
#include <string.h>
#include "cache.h"

void cacheInit(struct DNSCache* cache){
	for(int i = 0; i < cacheSize; i++) cache->entries[i].valid = 0;

	pthread_rwlock_init(&cache->lock, NULL);
}

int cacheLookup(struct DNSCache* cache, const char* domain, struct DNSRecord* result){
	pthread_rwlock_rdlock(&cache->lock);

	for(int i  = 0; i < cacheSize; i++){
		if(cache->entries[i].valid == 1 && strcmp(cache->entries[i].record.domain, domain) == 0){
		       	/*store dnsrecord into result*/
		       	*result = cache->entries[i].record;
			pthread_rwlock_unlock(&cache->lock);
			return 1; /*cache hit*/
		}
	}

	pthread_rwlock_unlock(&cache->lock);

	return 0; /*cache miss*/
}

void cacheInsert(struct DNSCache* cache, const struct DNSRecord* record){
	pthread_rwlock_wrlock(&cache->lock);

	int emptySlot = -1;

	for(int i = 0; i < cacheSize; i++){
		/*if domain exists alr, update it*/
		if(cache->entries[i].valid && strcmp(cache->entries[i].record.domain, record->domain) == 0){
			cache->entries[i].record = *record;
			pthread_rwlock_unlock(&cache->lock);
			return;
		}

		if(cache->entries[i].valid == 0 && emptySlot == -1) emptySlot = i; /* this will be the first empty slot*/
	}
	
	/*domain not found if we're here means we must inert in empty slot or beghar someone*/
	if(emptySlot != -1){
		cache->entries[emptySlot].record = *record;
		cache->entries[emptySlot].valid = 1;
	}
	else{
		/*begharing slot 0*/
		cache->entries[0].record = *record;
		cache->entries[0].valid = 1;
	}

	pthread_rwlock_unlock(&cache->lock);

	return;
}


void cacheDestroy(struct DNSCache* cache){
	/*to cleanup the rwlock during shutdown*/
	pthread_rwlock_destroy(&cache->lock);
}

void cacheInvalidate(struct DNSCache* cache, const char* domain){
	pthread_rwlock_wrlock(&cache->lock);

	for(int i  = 0; i < cacheSize; i++){
		if(cache->entries[i].valid == 1 && strcmp(cache->entries[i].record.domain, domain) == 0){
			cache->entries[i].valid = 0;
			break;
		}
	}

	pthread_rwlock_unlock(&cache->lock);
}


