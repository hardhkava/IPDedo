#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "auth.h"

/*file open error: -2
  exists already: -1*/

static pthread_mutex_t adminMutex = PTHREAD_MUTEX_INITIALIZER;

void authInit(void){
	pthread_mutex_lock(&adminMutex);
	FILE* fp = fopen("admins.csv", "r");
	if(fp == NULL){
		fp = fopen("admins.csv", "w");
		if(fp != NULL){
			fprintf(fp, "admin,password\n");
			fclose(fp);
			printf("[Auth] Created admins.csv with default super admin.\n");
		}
	} else {
		fclose(fp);
	}
	pthread_mutex_unlock(&adminMutex);
}

int authenticate(const char* username, const char* password){
	pthread_mutex_lock(&adminMutex);

	FILE* fp = fopen("admins.csv", "r");
	if(fp == NULL){
		pthread_mutex_unlock(&adminMutex);
		printf("file open error\n");
		return -2;
	}

	char line[256];
	char readUser[100], readPass[100];
	int found = 0;

	while(fgets(line, sizeof(line), fp)){
		line[strcspn(line, "\n")] = 0; /* repolaces \n from fgets at theend of line with 0 for strcmp to wortk perfectly*/

		if(sscanf(line, "%99[^,],%99s", readUser, readPass) == 2){
			if(strcmp(readUser, username) == 0 && strcmp(readPass, password) == 0){
				found = 1;
				break;
			}
		}
	}
	/*read each line, sscanf of %99 means read upto 99 characters, [^,] means stop when a comma is there, then password after that for 99 chars*/

	fclose(fp);
	pthread_mutex_unlock(&adminMutex);
	
	return found;
}


int registerAdmin(const char* username, const char* password){
	pthread_mutex_lock(&adminMutex);
	
	/* initial check for any duplicate username*/
	FILE* fp = fopen("admins.csv", "r");
	if(fp != NULL){
		char line[256];
		char currUser[100];
		while(fgets(line, sizeof(line), fp)){
			line[strcspn(line, "\n")] = 0;
			if(sscanf(line, "%99[^,]", currUser) == 1){
				if(strcmp(currUser, username) == 0){
					pthread_mutex_unlock(&adminMutex);
					fclose(fp);
					return -1; /*username exists already*/
				}
			}
		}
		fclose(fp); /*in case it doesnt exist*/
	}


	/* now we add new admin*/
	fp = fopen("admins.csv", "a");
	if(fp == NULL){
		pthread_mutex_unlock(&adminMutex);
		printf("file open error\n");
		return -2;
	}

	fprintf(fp, "%s,%s\n", username, password);
	fclose(fp);

	pthread_mutex_unlock(&adminMutex);

	return 0;
}


int allowed(enum Role role, const char* operation){
	if(role == ADMIN) return 1;
	if(role == USER){
		if(strcmp(operation, "query") == 0) return 1;
		else return 0;
	}
	return 0;
}



