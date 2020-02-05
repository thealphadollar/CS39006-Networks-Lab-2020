#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 80
#define PORT 8080

int main() { 
    int sockfd; 
    struct sockaddr_in servaddr, cliaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( sockfd < 0 ) { 
        perror("Unable to create Socket.");
        exit(EXIT_FAILURE);
    }
    else printf("Socket created.\n");
    bzero(&servaddr, sizeof(servaddr)); 


    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    //servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    
    if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) { 
        printf("Connection with the server failed.\n"); 
        exit(0); 
    } 
    else printf("Connected to the server...\n"); 
    
    
    
    int n;
    char requested_file[100];
    printf("Enter name of the file you want to request: ");
    scanf("%[^\n]%*c", requested_file);
    
    int temp_len = strlen(requested_file);
    
    requested_file[temp_len] = '$';
    requested_file[temp_len+1] = '\0';

    send(sockfd, requested_file, temp_len+1, 0); 
    printf("File request sent from client...\n");
   

    int i,j, flag;
    int total_bytes=0, total_words=0, cur_msg_index=0;
	char buffer[MAXLINE];
    char message[MAXLINE];
    flag = 0;

    int fd = open("received.txt", O_TRUNC|O_CREAT|O_WRONLY, 0777);
    if (fd == -1){
        printf("couldn't create file for writing received data!");
        exit(1);
    }

    // entire message counter
    i = 0;
    n = read(sockfd, buffer, MAXLINE);
    if (n == 0) {
        printf("File not found by server! Connection closed abruptyly!\n");
    }
    else {
        flag = 0;
        int is_non_delim = 0;
        do {
            printf("Received %d bytes\n",n);
            printf("Buffer: %s\n",buffer);
            for(j=0;j<n;j++)
            {
                if (j>0 && (buffer[j-1] == '~'))
                {
                    printf("Writing active...\n");
                    flag = 1;
                }
                if (buffer[j]=='$') 
                {
                    printf("Writing inactive!\n");
                    flag = 0;
                }
                if (flag) {
                    message[i] = buffer[j];
                    if (is_non_delim == 0){
                        if (message[i] != ',' && message[i] != ':' && message[i] != ';' && message[i] != '.' && message[i] != ' ' && message[i] != '\n' && message[i] != '\t'){
                            is_non_delim = 1;
                        }
                    }
                    else {
                        if (message[i] == ',' || message[i] == ':' || message[i] == ';' || message[i] == '.' || message[i] == ' ' || message[i] == '\n' || message[i] == '\t'){
                            is_non_delim = 0;
                            total_words++;
                        }
                    }
                    total_bytes++;
                    i++;
                }
            }
            message[i]='\0';
            printf("Message: %s", message);
            i = 0;
            write(fd, message, strlen(message));
            n = read(sockfd, buffer, MAXLINE);
        } while(n > 0);
    }
    printf("The file transfer is successful. Size of the file = %d bytes, no. of words = %d\n", total_bytes, total_words);
    close(sockfd);

    return 0; 
} 
