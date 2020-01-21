// A simple UDP server that sends a HELLO message
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAXLINE 1024

int main() {
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
	char * line = NULL;
    size_t llen = 0;
    ssize_t read;

	// Create socket file descriptor
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket creation failed!");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr,0,sizeof(servaddr));
	memset(&cliaddr,0,sizeof(cliaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	servaddr.sin_port = htons(8181);

	// Bind the socket with the server address
	if (bind(sockfd, (const struct  sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind failed!");
		exit(EXIT_FAILURE);
	}

	printf("Server running...\n");

	int n;
	socklen_t len;
	char buffer[MAXLINE];

	len = sizeof(cliaddr);
	n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
		(struct sockaddr *) &cliaddr, &len);
	buffer[n] = '\0';
	printf("requested file is %s\n", buffer);

	if (access(buffer, R_OK) != -1) {
		    FILE *fptr;
	    if ((fptr = fopen("program.txt", "r")) == NULL) {
	        printf("Error! opening file");
	        // Program exits if file pointer returns NULL.
	        exit(1);
	    }
	    else {
	    	while ((read = getline(&line, &llen, fptr)) != -1) {
		        printf("Retrieved line of length %zu:\n", read);
		        printf("%s", line);
				sendto(sockfd, line, strlen(line), 0, (const struct sockaddr *) &cliaddr, sizeof(&cliaddr));
    		}
	    }
	} else {
		printf("Here\n");
		// char *begin = "NOT FOUND ";
		// strcat(begin, buffer);
		sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (struct sockaddr *) &cliaddr, sizeof(&cliaddr));
	}
	return 0;
}