#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 5353

int main(){
	int sock = 0;
	struct sockaddr_in serverAddr;
	char buffer[1024];
	char input[1024];

	/*socket creation*/
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("socket creation error\n");
		return -1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);

	if(inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0){
		printf("Invalid address\n");
		return -1;
	}

	if(connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
		printf("connection to server failed\n");
		return -1;
	}


	/*loop for client*/
	while(1){
		memset(buffer, 0, sizeof(buffer));

		/*recieving message from server*/
		int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if(bytesRead <= 0){
			printf("server disconnected\n");
			break;
		}

		printf("%s", buffer);

		if(strstr(buffer, "disconnecting") != NULL) break;

		if(fgets(input, sizeof(input), stdin) == NULL) break;

		send(sock, input, strlen(input), 0);

		if(strncmp(input, "QUIT", 4) == 0) break;

	}

	close(sock);
	return 0;
}
