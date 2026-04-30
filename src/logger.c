#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include "logger.h"

#define logFile "server.log"

void loggerProcess(const char* pipePath){
	/*open log file for appending*/
	FILE* fp = fopen(logFile, "a");
	if(fp == NULL){
		printf("logger failed to open log file\n");
		exit(1);
	}

	/*open pipe in rdwr mode so read doesnt return 0 when client closes it*/
	int pipeFD = open(pipePath, O_RDWR);
	if(pipeFD == -1){
		printf("logger failed to open pipe\n");
		exit(1);
	}

	char buffer[1024];
	int bytesRead;

	/*infinite loop waiting for data from pipe*/
	while(1){
		bytesRead = read(pipeFD, buffer, sizeof(buffer) - 1);
		if(bytesRead > 0){
			buffer[bytesRead] = '\0';
			fprintf(fp, "%s", buffer);
			fflush(fp); /*flush imm to save to disk*/
		}
	}

	fclose(fp);
	close(pipeFD);
	exit(0);
}


void logEvent(const char* pipePath, const char* domain, const char* clientIP, const char* result){
	/*get curr time*/
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char timeStr[64];
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", t);

	/*format msg*/
	char logMsg[1024];
	snprintf(logMsg, sizeof(logMsg), "[%s] IP: %s | Query: %s | Result: %s\n", timeStr, clientIP, domain, result);

	/*open pipe, write msg, close*/
	int pipeFD = open(pipePath, O_WRONLY);
	if(pipeFD != -1){
		write(pipeFD, logMsg, strlen(logMsg));
		close(pipeFD);
	}
}

