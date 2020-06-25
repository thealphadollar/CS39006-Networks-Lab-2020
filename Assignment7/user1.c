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

 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    char *hello = "What a beautiful day today is!\0";
        
    struct sockaddr_in servaddr,cliaddr; 
  
    // Creating socket file descriptor 
    if ((sockfd = r_socket(AF_INET, SOCK_MRP, 0)) < 0 )
    { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    servaddr.sin_family    = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORTSERV);

    cliaddr.sin_family    = AF_INET;  
    cliaddr.sin_addr.s_addr = INADDR_ANY; 
    cliaddr.sin_port = htons(PORTCLI);
    
    if (r_bind(sockfd, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    
    int n;
    int len = strlen(hello); 
    int i = 0;
    for(i=0;i<len;++i)
    {
        buffer[0] = hello[i];
        n = r_sendto(sockfd, (const char*)buffer,1, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        buffer[n]='\0';
        printf("Sent:'%s'\n",buffer);
    }
    printf("Message sent.\n");
    
    while(1);
    r_close(sockfd);
    return 0; 
} 