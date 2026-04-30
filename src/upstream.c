#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "upstream.h"

/*dns format requires google.com to be 6google3com0*/
void formatDNSName(unsigned char* dns, unsigned char* host){
	int lock = 0 , i;
	strcat((char*)host,".");
	
	for(i = 0 ; i < strlen((char*)host) ; i++){
		if(host[i] == '.'){
			*dns++ = i - lock;
			for(; lock < i; lock++){
				*dns++ = host[lock];
			}
			lock++;
		}
	}
	*dns++ = '\0';
}

int fetchFromUpstream(const char* domain, char* ipOut){
	int sock;
	struct sockaddr_in dest;
	unsigned char buf[65536];

	/*udp socket creation*/
	sock = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
	if(sock < 0) return 0;
	
	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr.s_addr = inet_addr("8.8.8.8");

	/*dns header setup*/
	unsigned char *qname;
	buf[0] = 0x12; buf[1] = 0x34; /*id*/
	buf[2] = 0x01; buf[3] = 0x00; /*flags: standard query*/
	buf[4] = 0x00; buf[5] = 0x01; /*qdcount: 1*/
	buf[6] = 0x00; buf[7] = 0x00; /*ancount: 0*/
	buf[8] = 0x00; buf[9] = 0x00; /*nscount: 0*/
	buf[10]= 0x00; buf[11]= 0x00; /*arcount: 0*/

	qname = (unsigned char*)&buf[12];
	
	unsigned char host[256];
	strcpy((char*)host, domain);
	formatDNSName(qname, host);

	int qnameLen = strlen((const char*)qname) + 1;
	
	/*qtype A (1) and qclass IN (1)*/
	buf[12 + qnameLen] = 0x00; buf[13 + qnameLen] = 0x01;
	buf[14 + qnameLen] = 0x00; buf[15 + qnameLen] = 0x01;

	int queryLen = 12 + qnameLen + 4;

	/*send the udp packet*/
	if(sendto(sock, buf, queryLen, 0, (struct sockaddr*)&dest, sizeof(dest)) < 0){
		close(sock);
		return 0;
	}

	/*wait for the udp response*/
	socklen_t destLen = sizeof(dest);
	int recvLen = recvfrom(sock, buf, 65536, 0, (struct sockaddr*)&dest, &destLen);
	
	close(sock);

	if(recvLen < 0) return 0;

	/*check ancount (answer count)*/
	int ancount = (buf[6] << 8) | buf[7];
	if(ancount == 0) return 0;

	/*skip header and question to get to answer section*/
	int pos = queryLen;
	
	for(int i = 0; i < ancount; i++){
		/*skip name (usually 2 byte pointer starting with 11)*/
		if((buf[pos] & 0xC0) == 0xC0){
			pos += 2;
		} else {
			while(buf[pos] != 0) pos++;
			pos += 1;
		}

		int type = (buf[pos] << 8) | buf[pos+1];
		pos += 2;
		
		/*skip class and ttl*/
		pos += 6;

		int dataLen = (buf[pos] << 8) | buf[pos+1];
		pos += 2;

		if(type == 1){ /*A record found!*/
			sprintf(ipOut, "%d.%d.%d.%d", buf[pos], buf[pos+1], buf[pos+2], buf[pos+3]);
			return 1;
		}
		
		pos += dataLen; /*skip if not A record and try next answer*/
	}

	return 0;
}

