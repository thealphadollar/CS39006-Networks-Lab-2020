#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

// oldest connection is overwritten once the max connections are reached
#define MAX_CONNECTIONS 100
// A buffer size of 100Kb is required for good enough network speed
#define MAX_BUF_SIZE 100000


int inPORT, outPORT;
char outaddr[100];

char buff_duplicate[MAX_BUF_SIZE+1];

struct connection {
    int client, prox_server, len_addr;
};

void rec_socket(int *sockfdptr)
{
    // defining common variables for storing data
	int sockfd, connfd;
	socklen_t len;
    struct sockaddr_in servaddr;
    int multi_allow = 1;

    // Creating a TCP server socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    *sockfdptr = sockfd;
    if (sockfd < 0) { 
        printf("ERROR: Unable to create receiving socket.\n"); 
        exit(EXIT_FAILURE); 
    } 
    else printf("INFO: Receive Socket created.\n"); 

    // allow multiple incoming connections
    int multi_success = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&multi_allow, sizeof(multi_allow));
    if (multi_success<0)
    {
        printf("ERROR: Failed to allow multiple incoming connections!\n"); 
        exit(EXIT_FAILURE); 
    }

    
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
    else printf("INFO: Receiving Socket bound.\n");
    // Now tcp server is ready to listen and verification 

    if ((listen(sockfd, 5)) < 0) { 
        printf("ERROR: Receive Listen failed.\n"); 
        exit(0);
    }
    else printf("INFO: TCP Server listening...\n");
    return;
}

void send_socket(int *sockfdptr, struct sockaddr_in *servaddrptr, int outPORT, char* outaddr)
{   
    int sockfd; 
    struct sockaddr_in servaddr, myaddr; 
  
    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) {
        printf("ERROR: proxy server socket creation failed...\n"); 
        exit(0); 
    } 
    else printf("INFO: Proxy Server Socket successfully created..\n");

    bzero(&servaddr, sizeof(servaddr)); 
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(outaddr); 
    servaddr.sin_port = htons(outPORT); 
  
    // connect the client socket to server socket 
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) { 
        printf("ERROR: connection with the proxy server failed...\n"); 
        exit(0); 
    } 
    else printf("INFO: connected to the proxy server..\n"); 
    
    *sockfdptr = sockfd;
    *servaddrptr = servaddr;
}

// return the maximum file descriptor value
int max(int a, int b, struct connection * conns, int conn_val) 
{ 
    int cur_max = -1;
    if (a > b) 
        cur_max=a;
    else
        cur_max= b; 
    for(int i=0; i<conn_val;i++){
        if(conns[i].client>cur_max){
            cur_max=conns[i].client;
        }
        if(conns[i].prox_server>conns[i].client){
            cur_max = conns[i].prox_server;
        }
    }
    return cur_max;
}

//Function to parse and detect destination server of the HTTP requests
int parse_dest(char* buff, char* destIP, int* port, int* len_addr, int *https)
{
    int flag=0;
    char cur_word[1000], cur_line[1000];
    char *end_line;
    char *cur_line_dyno = strtok_r(buff, "\n", &end_line);

    //Iterating through the lines of the HTTP request
    //All information for determining destination server is present in the first line, 
    //thus this is the only line that is actually iterated through
    while(cur_line!=NULL){
        strcpy(cur_line, cur_line_dyno);
        //printf("DEBUG: CURLINE: %s\n",cur_line);
        char *end_word;
        char *cur_word_dyno = strtok_r(cur_line_dyno, " ", &end_word);
        //printf("DEBUG: CURLINE: %s\n",cur_line);
        
        //Iterating through the words (space seperated) of the lines
        while(cur_word!=NULL){
            strcpy(cur_word, cur_word_dyno);
            //printf("DEBUG: CURWORD: %s\n",cur_word);
            //Flag stops iteration from more than one line
            if (!flag){
                flag = 1;
            }
            //Obtaining the host name and then it's ip address through DNS
            //If HTTP request, port is assumed to be 80, unless further specified in the requests by the format "host:PORT"
            else if (flag){
                char *end_addr;
                char *destAddr = strtok_r(cur_word_dyno, ":", &end_addr);
                printf("DEBUG: CURWORD: %s\n",cur_word);
                struct hostent *resp;
                if (strcmp(destAddr, "http") == 0){
                    *len_addr = strlen(destAddr)-1;
                    *port = 80;
                    char destAddrBuff[1000];
                    int a=7, b = strlen(cur_word)-a-1;
                    strncpy(destAddrBuff, cur_word+a, b);
                    destAddrBuff[b]='\0';

                    int i, len_buff=strlen(destAddrBuff);

                    printf("DEBUG: Initial destAddrBuff: %s\n", destAddrBuff);

                    int colpos=len_buff;
                    for (i=0;i<len_buff;i++){
                        if (destAddrBuff[i]==':'){
                            destAddrBuff[i] = '\0';
                            colpos=i;
                        }
                        else if(destAddrBuff[i]== '/') {
                            destAddrBuff[i] = '\0';
                        }
                    }
                    i=colpos;
                    if (i<len_buff){
                        *port = atoi(destAddrBuff+i+1);
                    }
                    printf("DEBUG: Final destAddrBuff: %s\n", destAddrBuff);
                    // get IP from DNS
                    resp = gethostbyname(destAddrBuff);
                }
                //HTTPS requests are not handled
                else if (strcmp(destAddr, "https") == 0){
                    //Not handling https
                    *https=1;
                    break;
                    /*
                    *len_addr = strlen(destAddr)-1;
                    *port = 443;
                    char destAddrBuff[1000];
                    int a=8, b = strlen(cur_word)-a-1;
                    printf("%s",cur_word);
                    strncpy(destAddrBuff, cur_word+a, b);
                    printf("DEBUG: %s\n", destAddrBuff);
                    // get IP from DNS
                    resp = gethostbyname(destAddrBuff);
                    */
                }
                else {
                    *len_addr = strlen(destAddr)-1;
                    resp = gethostbyname(destAddr);
                    *port = atoi(strtok_r(NULL, ":", &end_addr));
                }
                // handle successful response
                if (resp)
                {
                    // convert an Internet network address into ASCII string
                    strcpy(destIP,inet_ntoa(*((struct in_addr*) 
                           resp->h_addr_list[0])));
                    //printf("DEBUG: %s:%d\n", destIP, *port);
                    if(*port==443) {
                        *https=1;
                        break;
                    }
                }
                // handle failed response
                else
                {
                    // put the error in herror
                    herror("gethostbyname");
                    destIP = destAddr;
                }
                break;
            }
            cur_word_dyno = strtok_r(NULL, " ", &end_word);
        }
        if (flag) break;
        cur_line_dyno = strtok_r(NULL, "\n", &end_line);
    }
}

//Function to modify the absolute path in the request given by the browser, to relative path when forwarding to server 
void modify_path(char* buffer_recv)
{
    int i=0, start=-1, start2=-1, count=3, len_buff=strlen(buffer_recv);
    char protocol[5];
    for(i=0;i<len_buff;++i)
        if(buffer_recv[i]==' ') break;
    i++;
    
    strncpy(protocol,buffer_recv+i,4);
    protocol[4]='\0';

    //Request is of two types: starts with "http://" or directly starts with website name
    //If starts with "http://", It is handled by rewriting the URI portion of buffer from the first '/' 
    //(it occurs after the host name and port (after authoritative path))
    if(strcmp(protocol,"http")==0){
        for (; i<strlen(buffer_recv); i++){
            if (buffer_recv[i]=='h' && start==-1) start=i;
            else if (buffer_recv[i]=='/') count--;
            if (count==0 && start2==-1) start2=i;
        }
        for (;start2<len_buff;start++,start2++){
            buffer_recv[start] = buffer_recv[start2];
        }
        buffer_recv[start] ='\0';
    }
    //If starts with website name, then the the URI portion of buffer is re written from the first '/' (occurs after website name amd port(authoritative path))
    else{
        for(;i<strlen(buffer_recv);++i){
            if(buffer_recv[i]=='/'){
                start=i;
                break;
            }
        }

        for(i=0;start<len_buff;i++,start++){
            buffer_recv[i]=buffer_recv[start];
        }

        buffer_recv[i]='\0';
    }
    
}


int main(int argc, char **argv)
{
    // ignore SIGPIPE as some TCP connection clients might become unavailable while sending data
    signal(SIGPIPE, SIG_IGN);

    if (argc<2)
    {
        printf("ERROR: Usage `./a.out PORT`\n");
        exit(EXIT_FAILURE);
    }
    // parsing command line arguments
	inPORT = atoi(argv[1]);
	printf("Proxy running on port %d.\n", inPORT);
    
    // defining variables
    struct sockaddr_in cliaddr, servaddr;
    int sockfd_cli, connfd_cli, sockfd_serv, len_cli_addr = sizeof(cliaddr), n,m, con_val=0, ready_fd, ndfs;
    char buffer_send[MAX_BUF_SIZE+1], buffer_recv[MAX_BUF_SIZE+1], std_buf[100];
    struct connection conns[MAX_CONNECTIONS];
    for (int i=0; i<MAX_CONNECTIONS;i++){
        conns[i].client=-1;
        conns[i].prox_server=-1;
    }

    // initiating listening on TCP
    printf("INFO: Creating TCP Receiving Socket...\n");
	rec_socket(&sockfd_cli);

    // initiate file descriptor set
    fd_set read_set, write_set;
    int con_count=0;
    while(1)
    {
        // using select() within a loop, the  sets  must  be  reinitialized before each call.
        FD_ZERO(&read_set);
        // attach reading incoming connection requests
        FD_SET(sockfd_cli, &read_set);
        // attach reading incoming stdin
        FD_SET(0, &read_set);
        // attach reading data from all connected requests; client and proxy server
        for(int i=0;i<con_val; i++) {
            if(conns[i].client>-1)
            {
                FD_SET(conns[i].client, &read_set);
                FD_SET(conns[i].prox_server, &read_set);
            }
        }

        // from man page, nfds should be set to the highest-numbered file descriptor in any of the three sets, plus 1
        int ndfs = max(0, sockfd_cli, conns, con_val) + 1;

        // get ready fd for read and write
        ready_fd = select(ndfs, &read_set, NULL, NULL, NULL);
        if (ready_fd<=0) continue;

        // read stdin
        if (FD_ISSET(0, &read_set))
        {
            // read user input
            scanf("%s", std_buf);
            if (strcmp(std_buf, "exit")==0)
            {
                printf("INFO: closing all connections...\n");
                // closing all connections
                close(sockfd_cli);
                close(sockfd_serv);
                close(connfd_cli);
                for(int i=0;i<MAX_CONNECTIONS;i++) {
                    // only close connections which were made
                    if (conns[i].client>-1)
                    {
                        close(conns[i].client);
                        close(conns[i].prox_server);
                    }
                }
                printf("INFO: Exiting!\n");
                return 0;
            }
        }

        // receiving new connection
        else if (FD_ISSET(sockfd_cli, &read_set))
        {
            // Accept the connection request from client
            connfd_cli = accept(sockfd_cli, (struct sockaddr *)&cliaddr, &len_cli_addr);
            // make it non-blocking
            fcntl(connfd_cli, F_SETFL, O_NONBLOCK);
            if (connfd_cli < 0) {
                printf("ERROR: server failed to accept client!\n");
                exit(EXIT_FAILURE);
            }
            else printf("INFO: Connection accepted from %s:%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
            conns[con_val].client = connfd_cli;
            // replace earliest connection with new one
            con_val = (con_val+1)%MAX_CONNECTIONS;
            con_count++;
            printf("INFO: Number handled connections: %d\n",con_count);
            continue;
        }

        //iterating through all tunnels and finding if any is sending information
        for(int i=0;i<MAX_CONNECTIONS;i++)
        {
            // read information from proxy server and pass to client
            if (conns[i].client>-1 && FD_ISSET(conns[i].prox_server, &read_set))
            {
                // clear buffer
                bzero(&buffer_send, sizeof(buffer_send));
                // receive data from proxy server
                m = recv(conns[i].prox_server, buffer_send, MAX_BUF_SIZE, 0);
                if(m>0)
                {
                    buffer_send[m]='\0';
                    printf("DEBUG: FROM SERVER: %s\n", buffer_send);
                    // tunnel data to client
                    send(conns[i].client, buffer_send, m, 0);
                }
            }
            // read information from client and pass to proxy server
            if (conns[i].client>-1 && FD_ISSET(conns[i].client, &read_set))
            {
                // clear buffer
                bzero(&buffer_recv, sizeof(buffer_recv));
                // receive data from client
                n = recv(conns[i].client, buffer_recv, MAX_BUF_SIZE, 0);
                if(n>0)
                {
                    buffer_recv[n]='\0';
                    // if this is first connection from this client, create connection to destination
                    printf("\nDEBUG: FROM_CLIENT: %s",buffer_recv);
                    if (conns[i].prox_server==-1)
                    {
                        strcpy(buff_duplicate, buffer_recv);
                        int flag=0;
                        parse_dest(buff_duplicate, outaddr, &outPORT, &conns[i].len_addr, &flag);
                        //This means https  (either has https:// or port is 443)
                        if(flag==1){
                            printf("NOT HANDLING HTTPS\n");
                            continue; 
                        }
                        
                        // create a tunnel to proxy server
                        printf("DEBUG: Tunnel connection to %s:%d\n",outaddr, outPORT);
                        send_socket(&sockfd_serv, &servaddr, outPORT, outaddr);
                        // make the tunnel non-blocking
                        fcntl(sockfd_serv, F_SETFL, O_NONBLOCK);
                        if (sockfd_serv<0) {
                            printf("ERROR: failed to connect to %s:%d server!\n", outaddr, outPORT);
                            exit(EXIT_FAILURE);
                        }
                        else printf("INFO: Tunnel created to destination %s:%d\n", outaddr, outPORT);
                        conns[i].prox_server = sockfd_serv;
                    }
                    
                    //Modify the path to convert it from absolute to relative
                    modify_path(buffer_recv);

                    // tunnel data to proxy server
                    printf("\nDEBUG: FROM_CLIENT: %s",buffer_recv);
                    int l = send(conns[i].prox_server, buffer_recv, n, 0);
                    printf("DEBUG: %d Sent data\n", l);
                }
            }
        }
    }
}