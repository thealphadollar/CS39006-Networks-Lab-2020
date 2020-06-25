#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "rsocket.h"

#define PORTSERV 52008
#define PORTCLI 52009

#define MAXLINE 1024 


// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE];
    char letter[1];
    struct sockaddr_in servaddr, cliaddr; 
    
    // Creating socket file descriptor 
    if ( (sockfd = r_socket(AF_INET, SOCK_MRP, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE);
    } 

    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    servaddr.sin_family    = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORTSERV);
    
    // Bind the socket with the server address 
    if (r_bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    printf("Server is running...\n");
    int len,n,i;
    int letter_count=0;
    for(i=0;i<30;++i)
    {
        n = r_recvfrom(sockfd, (char *)buffer, 1, 0, ( struct sockaddr *) &cliaddr, &len);
        if(n>0)
        {
            buffer[n]='\0';
            printf("Received '%s'\n",buffer);
        }
    }
    printf("Message Received\n");
    r_close(sockfd);
    return 0; 
} 