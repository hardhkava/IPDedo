#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdio.h>
#include <pthread.h>
#include <sempahore.h>

enum Role{ADMIN, USER};

struct DNSRecord{
	char domain[100]; //eg google.co.in
	char ip[16]; //12 digits, 3 dots, 1 \0
}

struct CacheEntry{
	struct DNSRecord record;
	int valid; // to check whether current array ele(cache) has a ip or not
};

struct DNSCache{
	struct CacheEntry entries[100];
	pthread_rwlock_t lock;//allows multiple threads to read, but only one to write
};

struct ClientDetails{
	int socketFD;
	enum Role role;
	char clientIP[16];
};
#endif



