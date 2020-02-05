// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT 8080
#define MAXLINE 1024
  
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // reset server address
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information and attributes setting
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    
    // define necessary variables
    int n;
    socklen_t len;
    len = sizeof(servaddr);
    char *requested_hostbyname;
    // get input for the domain from the user
    printf("Enter name you want to get host for: ");
    // only terminate at new line
    scanf("%[^\n]%*c", requested_hostbyname);

    // send request to the server
    sendto(sockfd, (const char *)requested_hostbyname, strlen(requested_hostbyname), 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr)); 
    printf("hostbyname request sent from client...\n"); 

    // accept response from the server
	char buffer[MAXLINE];
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
		(struct sockaddr *) &servaddr, &len);
	buffer[n] = '\0';
    printf("response from server is: %s\n", buffer);
    // free memory and close the file descriptor
    close(sockfd); 
    return 0; 
} 
