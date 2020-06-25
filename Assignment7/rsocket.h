#ifndef RSOCKET_H
#define RSOCKET_H

#include<stdlib.h>
#include<sys/socket.h>

#define P 0.1
#define INTERVAL 1
#define T 2
#define SOCK_MRP 235
#define ACK 0
#define APP 1

int r_socket(int domain, int type, int protocol);

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

int r_close(int fd);

/*
int dropMessage(float p);
*/

#endif