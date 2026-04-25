#ifndef STRUCTS_H
#define STRUCTS_H

#define domainLength 100
#define ipLength 16
#define cacheSize 100

#include <pthread.h>

enum Role{USER, ADMIN};

struct DNSRecord{
	char domain[domainLength]; //eg google.co.in
	char ip[ipLength]; //12 digits, 3 dots, 1 \0
};

struct CacheEntry{
	struct DNSRecord record;
	int valid; // to check whether current array ele(cache) has a ip or not
};

struct DNSCache{
	struct CacheEntry entries[cacheSize];
	pthread_rwlock_t lock;//allows multiple threads to read, but only one to write
};

struct ClientDetails{
	int socketFD;
	enum Role role;
	char clientIP[ipLength];
	struct DNSCache* cache;
};
#endif
