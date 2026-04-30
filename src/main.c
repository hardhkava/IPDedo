#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include "structs.h"
#include "cache.h"
#include "resolver.h"
#include "logger.h"

#define PORT 5353
#define maxClients 50
#define pipePath "/tmp/dns_pipe"

/*global sempahore for client connecs*/
sem_t clientSem;

struct DNSCache serverCache;

/* globals for signal handler to clean up */
int serverFD;
pid_t loggerPid = -1;

void handleSigint(int sig){
	printf("\n[Server] Caught SIGINT (Ctrl+C). Shutting down gracefully...\n");

	/*kill logger process*/
	if(loggerPid > 0){
		kill(loggerPid, SIGKILL);
		waitpid(loggerPid, NULL, 0); /*reap zombie*/
	}

	/*unlink named pipe*/
	unlink(pipePath);

	/*close server socket*/
	close(serverFD);

	/*cleanup cache and semaphores*/
	cacheDestroy(&serverCache);
	sem_destroy(&clientSem);

	printf("[Server] Cleanup complete. Goodbye!\n");
	exit(0);
}


int main(){
	int clientFD;
	struct sockaddr_in serverAddr, clientAddr;
	socklen_t clientLen = sizeof(clientAddr);

	/*register signal handler for Ctrl+C*/
	signal(SIGINT, handleSigint);

	/*init semaphore and cache*/
	sem_init(&clientSem, 0, maxClients);
	cacheInit(&serverCache);

	/*create named pipe for ipc (ignore error if it already exists)*/
	mkfifo(pipePath, 0666);

	/*fork logger process*/
	loggerPid = fork();
	if(loggerPid < 0){
		perror("fork failed\n");
		exit(1);
	}
	if(loggerPid == 0){
		/*child process becomes the logger*/
		signal(SIGINT, SIG_IGN);
		loggerProcess(pipePath);
	}

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
	printf("Press Ctrl+C to safely shut down the server.\n");


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

	/*we will never reach here because of the infinite loop, cleanup is handled by SIGINT*/
	return 0;
}

