#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include <arpa/inet.h>

extern int h_errno;

// define global constants
#define PORT 8080
#define MAXLINE 1024

// max function declaration
int max(int a, int b);

int main()
{
    // defining common variables for storing data
	int tcpsockfd, connfd, udpsockfd;
	socklen_t len;
    struct sockaddr_in servaddr, cliaddr; 

    // Creating a TCP server socket
    tcpsockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (tcpsockfd < 0) { 
        printf("Unable to create Stream socket.\n"); 
        exit(EXIT_FAILURE); 
    } 
    else printf("Stream Socket created.\n"); 
    
    // setting server address attributes for both UDP and TCP
	bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created TCP socket to given IP and verification 
    if ((bind(tcpsockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
	{ 
        printf("ERROR: Unable to bind stream socket \n"); 
        exit(EXIT_FAILURE);
    } 
    else printf("Stream Socket bound.\n");

    // Create a UDP server socket
    udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpsockfd < 0) {
		perror("datagram socket creation failed!");
		exit(EXIT_FAILURE);
	}
    // Binding newly created UDP socket to server address of TCP connection
    if (bind(udpsockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("ERROR: Unable to bind datagram socket \n"); 
        exit(EXIT_FAILURE);
    }
    else printf("Datagram socket bound.\n");


    // set fd variables and clear the descriptor set
    fd_set read_set; // The file descriptors listed in readfds will be watched to see if characters become available for reading
    int ready_fd; // stores the descriptor which is ready
    // from man page, nfds should be set to the highest-numbered file descriptor in any of the three sets, plus 1
    int ndfs = max(udpsockfd, tcpsockfd) + 1;
    FD_ZERO(&read_set);

    // looping for accepting connection
    while(1) 
    {
        // using select() within a loop, the  sets  must  be  reinitialized before each call.
        FD_SET(tcpsockfd, &read_set);
        FD_SET(udpsockfd, &read_set);

        // wait for either TCP or UDP descriptor to get ready
        ready_fd = select(ndfs, &read_set, NULL, NULL, NULL);
        
        // get length of client address
        len = sizeof(cliaddr);

        // check if UDP descriptor is set from select
        if (FD_ISSET(udpsockfd, &read_set))
        {
            // handle UDP connection in a child process
            int pid = fork();
            if (pid < 0) {
                printf("Failed to create child to handle UDP connection!\n");
            }
            else if (pid == 0) 
            {
                // define required variables including buffer
                int n;
                char buffer[MAXLINE];
                // set buffer to series of zeros
                bzero(buffer, sizeof(buffer)); 
                // receive the domain name from client
                n = recvfrom(udpsockfd, (char *)buffer, MAXLINE, 0,
                    (struct sockaddr *) &cliaddr, &len);
                buffer[n] = '\0';
                printf("requested domain is %s\n", buffer);

                // get the host
                struct hostent *resp = gethostbyname(buffer);
                // handle successful response
                if (resp)
                {
                    char *IP_ascii;
                    // convert an Internet network address into ASCII string 
                    IP_ascii = inet_ntoa(*((struct in_addr*) 
                           resp->h_addr_list[0]));
                    printf("Sending hostname %s with IP %s to client...\n", resp->h_name, IP_ascii);
                    // send ip address to client
                    sendto(udpsockfd, IP_ascii, strlen(IP_ascii), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
                }
                // handle failed response
                else
                {
                    // put the error in herror
                    herror("gethostbyname");
                    // create the error response string to send to client
                    char *begin = "SERVER ERROR: Failed To Get Host For Name: ";
                    char *to_buffer = malloc (strlen (begin) + strlen (buffer) + 1);
                    if (to_buffer == NULL) {
                        // handle memory failure
                        printf("ERROR: OUT OF MEMORY!!!\n");
                    } else {
                        // send failure message to client
                        printf("ERROR: Failed to get host by name\n");
                        strcpy(to_buffer, begin);
                        strcat(to_buffer, buffer);
                        sendto(udpsockfd, (const char *)to_buffer, strlen(to_buffer), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                        free(to_buffer);
                    }
                    // free malloc'ed memory
                    free(begin);
                }
                // closing the listening connection in child
                close(udpsockfd);
                close(connfd);
                return 0;
            }
        }

        // check if TCP descriptor is set from select
        if (FD_ISSET(tcpsockfd, &read_set))
        {
            // handle TCP connection
        }
    }

    return 0;
}


int max(int a, int b) 
{ 
    if (a > b) 
        return a; 
    else
        return b; 
} 