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

int inPORT, outPORT;
char outaddr[100];

void rec_socket(int *sockfdptr, int *connfdptr, struct sockaddr_in *cliaddrptr)
{
    // defining common variables for storing data
	int sockfd, connfd;
	socklen_t len;
    struct sockaddr_in servaddr, cliaddr; 

    // Creating a TCP server socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    *sockfdptr = sockfd;
    if (sockfd < 0) { 
        printf("Unable to create Stream socket.\n"); 
        exit(EXIT_FAILURE); 
    } 
    else printf("Stream Socket created.\n"); 
    
    // setting server address attributes for TCP
	bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(inPORT); 
  
    // Binding newly created TCP socket to given IP and verification 
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
	{ 
        printf("ERROR: Unable to bind stream socket \n"); 
        exit(EXIT_FAILURE);
    } 
    else printf("Stream Socket bound.\n");
    // Now tcp server is ready to listen and verification 

    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed.\n"); 
        exit(0);
    }
    else printf("TCP Server listening...\n");


    connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
    *connfdptr = connfd;
    *cliaddrptr = cliaddr;
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0);
    } 
    else printf("Server has accepted the client...\n"); 
    return;
}

void send_socket(int *sockfdptr, struct sockaddr_in *servaddrptr)
{   
    int sockfd; 
    struct sockaddr_in servaddr, myaddr; 
  
    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) {
        printf("Send socket creation failed...\n"); 
        exit(0); 
    } 
    else printf("Send Socket successfully created..\n"); 
    



    bzero(&servaddr, sizeof(servaddr)); 
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(outaddr); 
    servaddr.sin_port = htons(outPORT); 
  
    // connect the client socket to server socket 
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) { 
        printf("Send connection with the server failed...\n"); 
        exit(0); 
    } 
    else printf("Send connected to the server..\n"); 
    
    *sockfdptr = sockfd;
    *servaddrptr = servaddr;
}



int main(int argc, char **argv)
{
    if (argc<4)
    {
        printf("ERROR: Usage `./a.out PORT PROXY_ADDR PROXY_PORT`\n");
        exit(EXIT_FAILURE);
    }
	inPORT = atoi(argv[1]);
	strcpy(outaddr,argv[2]);
	outPORT = atoi(argv[3]);

	printf("Proxy running on port %d. Forwarding all connections to %s:%d\n", inPORT, outaddr, outPORT);
    
    int sockfd_cli, connfd_cli, sockfd_serv;
    struct sockaddr_in cliaddr, servaddr;

    printf("Receiving socket creation:\n");
	rec_socket(&sockfd_cli, &connfd_cli, &cliaddr);
    fcntl(connfd_cli, F_SETFL, O_NONBLOCK);
    printf("\nSending socket creation:\n");
    send_socket(&sockfd_serv, &servaddr);    
    fcntl(sockfd_serv, F_SETFL, O_NONBLOCK);

    char buffer[10001];
    int flag,n;
    while(1)
    {
        printf("in outer while\n");
        while(1)
        {
            // printf("in first while\n");
            n = recv(connfd_cli, buffer, 10000, 0);
            if(n<0)
            {
                if(flag>0)
                {
                    flag=0;
                    printf("Exiting1..\n");
                    break;
                } 
            }
            else if(n>0)
            {
                buffer[n]='\0';
                printf("%s",buffer);
                int l = send(sockfd_serv, buffer, n, 0);
                flag++;
            }
        }

        printf("HERE\n");
        while(1)
        {
            n = recv(sockfd_serv, buffer, 1000, 0);
            // printf("in second while\n");
            if(n<0)
            {
                if(flag>0) 
                {
                    flag=0;
                    printf("Exiting2..\n");
                    break;
                }
            }
            else if(n>0)
            {
                buffer[n]='\0';
                printf("%s", buffer);
                int l = send(connfd_cli, buffer, n, 0);
                flag++;
            }
        }
    }
    printf("out outer while\n");

    close(sockfd_cli);
    close(sockfd_serv);
	close(connfd_cli);
	return 0;
}