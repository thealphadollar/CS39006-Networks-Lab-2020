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
	    if ((fptr = fopen(buffer, "r")) == NULL) {
	        printf("Error opening file!\n");
	        // Program exits if file pointer returns NULL.
	        exit(1);
	    }
	    else {
			// capable of sending files with upto length accepted from code.
			char *req_begin = malloc(MAXLINE + 1);
			int word = 1;
	    	while ((read = getline(&line, &llen, fptr)) != -1) {
		        printf("Sending line (length %zu): ", read);
		        printf("%s\n", line);
				sendto(sockfd, line, strlen(line), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
				if (strcmp(line, "END") == 0 || strcmp(line, "END\n") == 0) {
					break;
				}
				memset(buffer, 0, sizeof(buffer));
				n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
					(struct sockaddr *) &cliaddr, &len);
				strcpy(req_begin, "WORD");
				char word_str[50];
				sprintf(word_str, "%d", word);
				strcat(req_begin, word_str);
				if (strcmp(buffer, req_begin) != 0) {
					printf("ERROR: wrong request from client \nreceived: %s | expected: %s\n", buffer, req_begin);
					sendto(sockfd, "ERROR: UNHANDLED REQUEST", strlen("ERROR: UNHANDLED REQUEST"), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
					break;
				}
				word += 1;
    		}
			printf("SUCCESS: all lines sent!\n");
			free(req_begin);
	    }
	} else {
		char *begin = "NOTFOUND ";
		char *to_buffer = malloc (strlen (begin) + strlen (buffer) + 1);
		if (to_buffer == NULL) {
			printf("ERROR: OUT OF MEMORY!!!\n");
		} else {
			printf("ERROR: Requested file not found!\n");
			strcpy(to_buffer, begin);
			strcat(to_buffer, buffer);
			sendto(sockfd, (const char *)to_buffer, strlen(to_buffer), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
			free(to_buffer);
		}
	}
	return 0;
}