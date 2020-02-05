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

#define MAXLINE 1024
#define PORT 4000

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

    // set socket file attributes
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    //servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    
    if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) { 
        printf("Connection with the server failed.\n"); 
        exit(0); 
    } 
    else printf("Connected to the server...\n");    
    
    // get user input
    int n;
    char requested_dir[100];
    printf("Enter name of the sub directory you want to request: ");
    scanf("%[^\n]%*c", requested_dir);
    // char requested_dir[] = "im0";

    int temp_len = strlen(requested_dir);
    
    // end filename with $
    requested_dir[temp_len] = '$';
    requested_dir[temp_len+1] = '\0';

    // send directory name
    send(sockfd, requested_dir, temp_len+1, 0); 
    printf("Directory request sent from client...\n");
   

    int i,j, flag, num_file = 1;
    int cur_msg_index=0;
	char buffer[MAXLINE];
    char img_data[MAXLINE];
    flag = 0;
    char image_filename[12];

    // creating name of the received image
    bzero(image_filename, sizeof(image_filename));
    snprintf(image_filename, sizeof(image_filename), "recv_%d.jpg", num_file);
    // creating / opening file if it already exists with truncation
    int fd = open(image_filename, O_TRUNC|O_CREAT|O_WRONLY, 0777);
    if (fd == -1){
        // handle file opening / creating error
        printf("couldn't create file for writing received data!");
        exit(1);
    }

    i = 0;
    // read first sent bytes
    n = read(sockfd, buffer, MAXLINE);
    if (n == 0) {
        printf("Directory not found by server! Connection closed abruptyly!\n");
        exit(EXIT_FAILURE);
    }
    else {
        flag = 1;
        // countdown to wait for EOF to pass to start writing the next file.
        int countdown = 3;
        int to_exit = 0;
        do {
            // printf("Received %d bytes\n",n);
            for(j=0;j<n;j++)
            {
                // printf("%c%c%c\n", buffer[j], buffer[j+1], buffer[j+2]);
                // data segment of next file begins
                if (!flag) {
                    countdown--;
                    if (!countdown){
                        printf("EOF passed, writing active!\n");
                        flag = 1;
                        countdown = 3;
                    }
                }

                // received end of entire data
                if ((buffer[j] == 'E') && (buffer[j+1] == 'N') && (buffer[j+2] == 'D'))
                {
                    printf("Entire data received!\n");
                    close(fd);
                    remove(image_filename);
                    num_file--;
                    break;
                    to_exit = 1;
                }

                // received end of current file
                if ((buffer[j] == 'E') && (buffer[j+1] == 'O') && (buffer[j+2] == 'F'))
                {
                    printf("Writing inactive, current image file has ended!\n");
                    // printf("value of i: %d", i);
                    write(fd, img_data, i);
                    close(fd);
                    bzero(img_data, sizeof(img_data));
                    i = 0;
                    num_file++;

                    // changing file to next file
                    snprintf(image_filename, sizeof(image_filename), "recv_%d.jpg", num_file);
                    // creating / opening file if it already exists with truncation
                    int fd = open(image_filename, O_TRUNC|O_CREAT|O_WRONLY, 0777);
                    if (fd == -1){
                        // handle file opening / creating error
                        printf("couldn't create file for writing received data!");
                        exit(1);
                    }

                    flag = 0;
                }
                if (flag) {
                    img_data[i] = buffer[j];
                    i++;
                }
            }
            // message[i]='\0';
            // printf("Message: %s", message);
            // printf("value of i: %d", i);
            if (to_exit) break;
            // printf("img_data: %d\n", strlen(img_data));
            write(fd, img_data, i);
            i = 0;
            n = read(sockfd, buffer, MAXLINE);
        } while(n > 0);
    }
    printf("The sub-directory transfer is successful. No. of imagess = %d\n", num_file);
    close(sockfd);
    return 0; 
} 
