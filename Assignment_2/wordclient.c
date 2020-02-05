// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

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
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t len;
    len = sizeof(servaddr);
    char *requested_file;
    printf("Enter name of the file you want to request: ");
    scanf("%[^\n]%*c", requested_file);

    sendto(sockfd, (const char *)requested_file, strlen(requested_file), 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr)); 
    printf("file request sent from client...\n"); 

	char buffer[MAXLINE];
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
		(struct sockaddr *) &servaddr, &len);
	buffer[n] = '\0';
    // printf("message from server is %s\n", buffer);
    // printf("%s\n", buffer);
    if (strcmp(buffer, "HELLO\n") == 0) {
        FILE *fptr;
        // writing data received from server in recv.txt
        fptr = fopen("recv.txt", "w");
        // exiting program on not being able to open file
        if (fptr == NULL) {
            printf("Error!\n");
            exit(1);
        }
        fprintf(fptr, "%s", buffer);

        char *request = malloc(MAXLINE + 1);
        int word = 1;
        do {
            strcpy(request, "WORD");
            char word_str[50];
            sprintf(word_str, "%d", word);
            strcat(request, word_str);
            printf("Sending request (length %ld): ", strlen(request));
            printf("%s\n", request);
            sendto(sockfd, (const char *)request, strlen(request), 0, 
			    (const struct sockaddr *) &servaddr, sizeof(servaddr));
            memset(buffer, 0, sizeof(buffer));
            n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
                (struct sockaddr *) &servaddr, &len);
            buffer[n] = '\0';
            printf("Received response: %s\n", buffer);
            if (strcmp(buffer, "ERROR: UNHANDLED REQUEST") == 0) {
                printf("ERROR: wrong request sent from client\n");
                break;
            }
            fprintf(fptr, "%s", buffer);
            word += 1;
        } while(strcmp(buffer, "END") != 0 && strcmp(buffer, "END\n") != 0);
        printf("Received file from server written to recv.txt\n");
        free(request);
        fclose(fptr);
    }
    else {
        char *begin = "NOTFOUND ";
        char *to_buffer = malloc (strlen (begin) + strlen (requested_file) + 1);
        if (to_buffer == NULL) {
            printf("ERROR: OUT OF MEMORY!!!\n");
        } else {
            strcpy(to_buffer, begin);
            strcat(to_buffer, requested_file);
            if (strcmp(to_buffer, buffer) == 0) {
                printf("File %s Not Found\n", requested_file);
            }
            else {
                printf("UNKNOWN ERROR!\n");
            }
            free(to_buffer);
        }
    } 
    close(sockfd); 
    return 0; 
} 
