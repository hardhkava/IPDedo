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
#include "logger.h"
#include "resolver.h"
#include "upstream.h"

#define pipePath "/tmp/dns_pipe"

extern sem_t clientSem;

int resolve(struct DNSCache* cache, const char* domain, char* res){
	struct DNSRecord record;

	/*check cache*/
	if(cacheLookup(cache, domain, &record)){
		strcpy(res, record.ip);
		return 1; /*found in cache */
	}

	/*check records.csv*/
	if(findRecord(domain, &record)){
		strcpy(res, record.ip);
		cacheInsert(cache, &record);
		return 2; /*found in records */
	}

	/*check upstream dns 8.8.8.8 via udp*/
	if(fetchFromUpstream(domain, res)){
		strcpy(record.domain, domain);
		strcpy(record.ip, res);
		cacheInsert(cache, &record);
		return 3; /*found upstream */
	}

	return 0; /*miss */
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

		if (authenticate(username, password) != 1) {
            		strcpy(response, "Authentication failed,disconnecting...\n");
            		send(client->socketFD, response, strlen(response), 0);
            		goto cleanup;
        	}

		strcpy(response, "Authentication successful, welcome admin ji\n");
	}

	else{
		client->role = USER;
		strcpy(response, "Welcome user ji\n");
	}

	

	/*interactive command loop*/
	while(1){
		char prompt[512];
		if(client->role == ADMIN) strcpy(prompt, "\nCommands: REGISTER <user> <pass>, QUERY <domain>, ADD <domain> <ip>, DELETE <domain>, QUIT\n> ");
		else strcpy(prompt, "\nCommands: QUERY <domain>, QUIT\n> ");

		strcat(response, prompt);

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
				int status = resolve(client->cache, arg1, ip);
				
				if(status == 1){
					sprintf(response, "Found %s -> %s Source: DNS cache\n", arg1, ip);
					logEvent(pipePath, arg1, client->clientIP, "HIT (Cache)");
				}
				else if(status == 2){
					sprintf(response, "Found %s -> %s Source: Records.csv\n", arg1, ip);
					logEvent(pipePath, arg1, client->clientIP, "HIT (Records)");
				}
				else if(status == 3){
					sprintf(response, "Found %s -> %s Source: 8.8.8.8 UDP QUery\n", arg1, ip);
					logEvent(pipePath, arg1, client->clientIP, "HIT (Upstream)");
				}
				else{
					sprintf(response, "ip not found %s\n", arg1);
					logEvent(pipePath, arg1, client->clientIP, "MISS");
				}
			}
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
				logEvent(pipePath, arg1, client->clientIP, "ADDED RECORD");
			}
		}

		else if(parsed == 3 && strcmp(cmd, "REGISTER") == 0){
            		if(client->role != ADMIN) strcpy(response, "permission denied\n");
            		else{
                		int result = registerAdmin(arg1, arg2);
                		if (result == 0) strcpy(response, "New admin registered successfully\n");
				else if (result == -1) strcpy(response, "Username already exists\n");
                		else strcpy(response, "Error opening admins.csv\n");
			}

        	}


		else if(parsed >= 2 && strcmp(cmd, "DELETE") == 0){
			if(allowed(client->role, "deleteRecord") == 0) strcpy(response, "permission denied\n");
			else{
				if(deleteRecord(arg1) == 0){
					cacheInvalidate(client->cache, arg1);
					strcpy(response, "Record deleted\n");
					logEvent(pipePath, arg1, client->clientIP, "DELETED RECORD");
				}
				else strcpy(response, "record not found to delete\n");
			}
		}


		else{
			strcpy(response, "Invalid command\n");
		}
	}

cleanup:
	close(client->socketFD);
	free(client);
	sem_post(&clientSem); /*releasing the slot*/

	return NULL;
}	

