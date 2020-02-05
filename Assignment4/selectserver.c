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
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>

extern int h_errno;

// define global constants
#define PORT 4000
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
    // Now tcp server is ready to listen and verification 
    if ((listen(tcpsockfd, 5)) != 0) { 
        printf("Listen failed.\n"); 
        exit(0);
    }
    else printf("TCP Server listening...\n"); 

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

    // looping for accepting connection
    while(1) 
    {
        // using select() within a loop, the  sets  must  be  reinitialized before each call.
        FD_ZERO(&read_set);
        FD_SET(udpsockfd, &read_set);
        FD_SET(tcpsockfd, &read_set);

        // wait for either TCP or UDP descriptor to get ready
        ready_fd = select(ndfs, &read_set, NULL, NULL, NULL);
        // if ready_fd returns a non-valid fd
        if (ready_fd <= 0) continue;

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
                return 0;
            }
        }

        // check if TCP descriptor is set from select
        if (FD_ISSET(tcpsockfd, &read_set))
        {
            // handle TCP connection in a child process
            int pid = fork();
            if (pid < 0) 
            {
                printf("Failed to create child to handle TCP connection!\n");
            }
            else if (pid == 0) 
            {
                // handle TCP connection
                // Accept the data packet from client and verification 
                connfd = accept(tcpsockfd, (struct sockaddr *)&cliaddr, &len); 
                if (connfd < 0) { 
                    printf("TCP server acccept failed...\n"); 
                    exit(EXIT_FAILURE); 
                } 
                else printf("Server has accepted the TCP client...\n");

                int i,j,n, flag;
                char buffer[MAXLINE];
                char req_sub_dir[MAXLINE];

                // receive the name of the folder inside images
                flag=0;
                i=0;
                while(1)
                {
                    n = recv(connfd, (char *)buffer, MAXLINE, 0);
                    if(n>0)
                    {
                        for(j=0;j<n;j++,i++)
                        {
                            // $ is the symbol to terminate on
                            if(buffer[j]=='$') 
                            {
                                req_sub_dir[i]='\0';
                                flag=1;
                                break;
                            }
                            else req_sub_dir[i] = buffer[j];
                        }
                        
                    }
                    if(flag==1) break;
                }
                printf("requested image sub directory is %s\n", req_sub_dir);

                // create the directory path string
                char *begin = "images/";
                char *to_path = malloc (strlen (begin) + strlen (req_sub_dir) + 1);
                if (to_path == NULL) {
                    // handle memory failure
                    printf("ERROR: OUT OF MEMORY!!!\n");
                    exit(EXIT_FAILURE);
                }
                strcpy(to_path, begin);
                strcat(to_path, req_sub_dir);

                // checking for directory existence in images folder
                DIR* dir = opendir(to_path);
                if (dir) {
                    /* Directory exists. */
                    char send_buffer[2*MAXLINE];
                    struct dirent* in_file;
                    int cur_file, k;
                    // read each file in the directory
                    while ((in_file = readdir(dir)))
                    {
                        // skipping . and .. files which are for movement 
                        if (!strcmp (in_file->d_name, "."))
                            continue;
                        if (!strcmp (in_file->d_name, ".."))
                            continue;

                        printf("sending file %s...\n", in_file->d_name);

                        // create full path to the image
                        // create the directory path string
                        char *full_img_path = malloc (strlen (to_path) + strlen (in_file->d_name) + 2);
                        if (full_img_path == NULL) {
                            // handle memory failure
                            printf("ERROR: OUT OF MEMORY!!!\n");
                            exit(EXIT_FAILURE);
                        }
                        strcpy(full_img_path, to_path);
                        strcat(full_img_path, "/");
                        strcat(full_img_path, in_file->d_name);

                        // open file to send it in packets of length MAXLINE+2
                        cur_file = open(full_img_path, O_RDONLY);
                        k=0;
                        while(1)
                        {
                            // set buffer to series of zeros
                            bzero(send_buffer, sizeof(send_buffer)); 
                            k = read(cur_file, send_buffer, MAXLINE);
                            // printf("sending %d bytes\n", k);
                            // mark end of one image file with EOF
                            if (k<MAXLINE)
                            {
                                send_buffer[k] = 'E';
                                send_buffer[k+1] = 'O';
                                send_buffer[k+2] = 'F';
                                send(connfd, send_buffer, k+3, 0);
                                printf("file sent!\n");
                                break;
                            }
                            // only send k bytes if not end of file
                            send(connfd, send_buffer, k, 0);
                        }
                    }
                    // send end message
                    send(connfd, "END", sizeof("END"), 0);
                    closedir(dir);
                } else {
                    /* Directory does not exist. */
                    printf("Subdirectory not found.\n");
                    close(connfd);
                    exit(EXIT_FAILURE);
                }
                close(connfd);
                close(tcpsockfd);
                free(to_path);
            }
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