#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include "resolver.h"
#include "auth.h"
#include "cache.h"
#include "records.h"

int resolve(struct DNSCache* cache, const char* domain, char* res){
	struct DNSRecord record;

	/*check cache*/
	if(cacheLookup(cache, domain, &record)){
		strcpy(res, record.ip);
		return 1;
	}

	/*check records.csv*/
	if(findRecord(domain, &record)){
		strcpy(res, record.ip);
		cacheInsert(cache, &record); /*add to cache cuz it missed*/
		return 1;
	}

	return 0;
}


void* handleClient(void* arg){
	struct ClientDetails* client = (struct ClientDetails*) arg;
	char buffer[512];
	char response[512];
	int bytesRead;


	/*Role selection prompt*/
	strcpy(response, "Select your role:\n 1->User \n 2->Admin\n>");
	send(client->socketFD, response, strlen(response), 0);

	bytesRead = recv(client->socketFD, buffer, sizeof(buffer) - 1, 0);
	if(bytesRead <= 0) goto cleanup;

	buffer[bytesRead] = '\0';
	buffer[strcspn(buffer, "\r\n")] = '\0'; /*strip newline*/


	
	/*if admin is there then authenticate, default to user*/
	if(strcmp(buffer, "2") == 0){
		client->role = ADMIN;

		/*authenticate admin*/
		strcpy(response, "Username: ");
		send(client->socketFD, response, strlen(response), 0);

		bytesRead = recv(client->socketFD, buffer, sizeof(buffer) - 1, 0);
		if(bytesRead <= 0) goto cleanup;

		buffer[strcspn(buffer, "\r\n")] = '\0';

		char username[100];
		strcpy(username, buffer);

		strcpy(response, "Password: ");
		send(client->socketFD, response, strlen(response), 0);

		bytesRead = recv(client->socketFD, buffer, sizeof(buffer) - 1, 0):
			if(bytesRead <= 0) goto cleanup;

		buffer[strcspn(buffer, "\r\n")] = '\0';

		char password[100];
		strcpy(password, buffer);

		if (authenticate(username, password) == 0) {
            		strcpy(response, "Authentication failed,disconnecting...\n");
            		send(ctx->socketFD, response, strlen(response), 0);
            		goto cleanup;
        	}

		strcpy(response, "Authentication successful, welcome admin ji\n");
		send(client->socketFD, response, strlen(response), 0);
	}

	else{
		client->role = USER;
		strcpy(response, "Welcome user ji\n");
		send(client->socketFD, response, strlen(response), 0);
	}


