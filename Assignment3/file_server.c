#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#define MAXLINE 50
#define PACKETSIZE 3
#define PORT 8080


int main() {
	int sockfd, connfd;
	socklen_t len;

    struct sockaddr_in servaddr, cliaddr; 
  
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) { 
        printf("Unable to create socket.\n"); 
        exit(0); 
    } 
    else printf("Socket created.\n"); 
    
	bzero(&servaddr, sizeof(servaddr)); 
  
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
	{ 
        printf("Unable to bind socket \n"); 
        exit(0);
    } 
    else printf("Socket bound.\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed.\n"); 
        exit(0);
    }
    else printf("Server listening...\n"); 
    len = sizeof(cliaddr); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else printf("Server has accepted the client...\n"); 
  
	int i,j,n, flag;
	char buffer[MAXLINE];
	char req_file[MAXLINE];

	flag=0;
	i=0;
	while(1)
	{
		n = recv(connfd, (char *)buffer, MAXLINE, 0);
		if(n>0)
		{
			for(j=0;j<n;j++,i++)
			{
				if(buffer[j]=='$') 
				{
					req_file[i]='\0';
					flag=1;
					break;
				}
				else req_file[i] = buffer[j];
			}
			
		}
		if(flag==1) break;
	}

	printf("requested file is %s\n", req_file);
	

	int file = open(req_file,O_RDONLY);
	if(file==-1)
	{
		printf("File not found.\n");
		close(sockfd);
		return 0;
	}
	int k=0;
	while(1)
	{
		k = read(file, buffer+1, PACKETSIZE);
		printf("%d\n", k);
		buffer[0] = '~';
		buffer[k+1] = '$';
		buffer[k+2] = '\0';
		printf("%s",buffer);
		send(connfd, buffer, k+2, 0);
		if(k<PACKETSIZE) break;
	}

    close(sockfd);

	return 0;
}