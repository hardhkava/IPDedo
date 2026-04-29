#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "structs.h"
#include "cache.h"
#include "resolver.h"

#define PORT 5353
#define maxClients 50

/*global sempahore for client connecs*/
sem_t clientSem;

struct DNSCache serverCache;

int main(){
	int serverFD, clientFD;
	struct sockaddr_in serverAddr, clientAddr;
	socklen_t clientLen = sizeof(clientAddr);

	/*init semaphore and client*/
	sem_init(&clientSem, 0, maxClients);

	cacheInit(&serverCache);


	/*create socket, tcp because we want persistent connecn, else would have to send user, pass for each packet*/
	serverFD = socket(AF_INET, SOCK_STREAM, 0);
	if(serverFD == -1){
		perror("socket creation failed\n");
		exit(1);
	}


	/* Prevent "Address already in use" errors if you restart the server quickly */
    	int opt = 1;
    	setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));



	/*binding socket*/
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);

	if(bind(serverFD, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
		perror("bind failed\n");
		exit(1);
	}


	/*listen*/
	if(listen(serverFD, 10) < 0){
		perror("listen failed\n");
		exit(1);
	}

	printf("IPDedo DNS Server running on port %d\n", PORT);


	/*accept loop*/
	while(1){
		sem_wait(&clientSem);

		clientFD = accept(serverFD, (struct sockaddr*)&clientAddr, &clientLen);
		if(clientFD < 0){
			perror("client accepting failed");
			sem_post(&clientSem); /*release slot as accept failed*/
			continue;
		}

		printf("new client connected: %s\n", inet_ntoa(clientAddr.sin_addr));

		/*creating client details struct*/
		struct ClientDetails* client = malloc(sizeof(struct ClientDetails));
		client->socketFD = clientFD;
		client->cache = &serverCache;
		
		strcpy(client->clientIP, inet_ntoa(clientAddr.sin_addr));


		/*create new thread to handle the client*/
		pthread_t tid;
		pthread_create(&tid, NULL, handleClient, client);

		/*detaching it to let it cleanup itself*/
		pthread_detach(tid);
	}

	


	close(serverFD);
	cacheDestroy(&serverCache);
	sem_destroy(&clientSem);

	return 0;
}
