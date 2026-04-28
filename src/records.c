#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "records.h"

static pthread_mutex_t recordMutex = PTHREAD_MUTEX_INITIALIZER;

int findRecord(const char* domain, struct DNSRecord *res){
	pthread_mutex_lock(&recordMutex);

	FILE* fp = fopen("records.csv", "r");
	if(fp == NULL){
		pthread_mutex_unlock(&recordMutex);
		printf("Error opening file\n");
		return 0;
	}

	char line[512];
	char readDomain[domainLength], readIP[ipLength];
	int found = 0;

	while(fgets(line, sizeof(line), fp)){
		line[strcspn(line, "\n")] = 0;

		if(sscanf(line, "%99[^,],%15s", readDomain, readIP) == 2){
			if(strcmp(readDomain, domain) == 0){
				strcpy(res->domain, readDomain);
				strcpy(res->ip, readIP);
				found = 1;
				break;
			}
		}
	}

	fclose(fp);

	pthread_mutex_unlock(&recordMutex);

	return found;
}

int saveRecord(const struct DNSRecord* record){
	pthread_mutex_lock(&recordMutex);

	FILE* fp = fopen("records.csv", "a");
	if(fp == NULL){
		pthread_mutex_unlock(&recordMutex);
		printf("Error opening file\n");
		return 0;
	}

	fprintf(fp, "%s,%s\n", record->domain, record->ip);

	fclose(fp);
	pthread_mutex_unlock(&recordMutex);

	return 0;
}

int deleteRecord(const char* domain){
	pthread_mutex_lock(&recordMutex);

	FILE* fp = fopen("records.csv", "r");
	if(fp == NULL){
                pthread_mutex_unlock(&recordMutex);
                printf("Error opening file\n");
                return -2;
        }
	

	FILE* tempFP = fopen("tempRecords.csv", "w");
	if(tempFP == NULL){
		fclose(fp);
                pthread_mutex_unlock(&recordMutex);
                printf("Error opening file\n");
                return -2;
        }


	char line[512];
	char readDomain[domainLength], readIP[ipLength];
	int deleted = 0;

	while(fgets(line, sizeof(line), fp)){
		char ogLine[512];
		strcpy(ogLine, line); /*keep og line for writing back*/

		line[strcspn(line, "\n")] = 0;

		if(sscanf(line, "%99[^,],%15s", readDomain, readIP) == 2){
			if(strcmp(readDomain, domain) == 0){
				deleted = 1;
				continue; /* wont write this to new file*/
			}
		}

		fputs(ogLine, tempFP);
	}

	fclose(fp);
	fclose(tempFP);

	/*now change new files name back to records.csv*/
	if(deleted == 1){
		remove("records.csv");
		rename("tempRecords.csv", "records.csv");
	}
	else remove("tempRecords.csv"); /*deleted bnothing so remove temp*/

	pthread_mutex_unlock(&recordMutex);

	if(deleted == 1) return 0;
	else return -1; 
}



