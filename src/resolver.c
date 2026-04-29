#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include "resolver.h"
#include "auth.h"
#include "cache.h"
#include "records.h"

extern sem_t clientSem;

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

		bytesRead = recv(client->socketFD, buffer, sizeof(buffer) - 1, 0);
		if(bytesRead <= 0) goto cleanup;

		buffer[strcspn(buffer, "\r\n")] = '\0';

		char password[100];
		strcpy(password, buffer);

		if (authenticate(username, password) == 0) {
            		strcpy(response, "Authentication failed,disconnecting...\n");
            		send(client->socketFD, response, strlen(response), 0);
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

	


	/*interactive command loop*/
	while(1){
		if(client->role == ADMIN) strcpy(response, "\nCommands: QUERY <domain>, ADD <domain> <ip>, DELETE <domain>, QUIT\n> ");
		else strcpy(response, "\nCommands: QUERY <domain>, QUIT\n> ");

		send(client->socketFD, response, strlen(response), 0);

		bytesRead = recv(client->socketFD, buffer, sizeof(buffer) - 1, 0);
		if(bytesRead <= 0) break;
		buffer[bytesRead] = '\0';
		buffer[strcspn(buffer, "\r\n")] = '\0';

		char cmd[20], arg1[100], arg2[20];
		int parsed = sscanf(buffer, "%19s %99s %19s", cmd, arg1, arg2);

		if(parsed >= 1 && strcmp(cmd, "QUIT") == 0) break;

		else if(parsed >= 2 && strcmp(cmd, "QUERY") == 0){
			if(allowed(client->role, "query") == 0) strcpy(response, "permission denied\n");

			else{
				char ip[ipLength];
				if(resolve(client->cache, arg1, ip)) sprintf(response, "Found %s -> %s\n", arg1, ip);
				else sprintf(response, "ip not found %s\n", arg1);
			}

			send(client->socketFD, response, strlen(response), 0);

		}

		else if(parsed == 3 && strcmp(cmd, "ADD") == 0){
			if(allowed(client->role, "addRecord") == 0) strcpy(response, "permission denied\n");
			else{
				struct DNSRecord rec;
				strcpy(rec.domain, arg1);
				strcpy(rec.ip, arg2);

				saveRecord(&rec);
				cacheInsert(client->cache, &rec);
				
				strcpy(response, "Record added\n");
			}

			send(client->socketFD, response, strlen(response), 0);
		}

		else if(parsed >= 2 && strcmp(cmd, "DELETE") == 0){
			if(allowed(client->role, "deleteRecord") == 0) strcpy(response, "permission denied\n");
			
			else{
				if(deleteRecord(arg1) == 0){
					cacheInvalidate(client->cache, arg1);
					strcpy(response, "Record deleted\n");
				}
				else strcpy(response, "record not found to delete\n");
			}

			send(client->socketFD, response, strlen(response), 0);

		}


		else{
			strcpy(response, "Invalid command\n");
			send(client->socketFD, response, strlen(response), 0);
		}
	}

cleanup:
	close(client->socketFD);
	free(client);
	sem_post(&clientSem); /*releasing the slot*/

	return NULL;
}	
		


