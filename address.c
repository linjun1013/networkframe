#define _GNU_SOURCE
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>

#include<fcntl.h>
#include<errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include"address.h"


ListenInfo *init_address(const char* addr, unsigned int port, int backlog, uint8_t ssl)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0)
        return NULL;

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(addr == NULL)
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else
        inet_pton(AF_INET, addr, &(server_addr.sin_addr.s_addr));

    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if(listen(sockfd, backlog) != 0){
        printf("listen fd %d failed exit\n", sockfd);
        return NULL;
    }

    ListenInfo *info = (ListenInfo*)malloc(sizeof(ListenInfo));
    info->fd = sockfd;
    info->ssl = ssl;

    return info;
}

ListenInfo* init_address_v6(const char * addr, unsigned int port, int backlog, uint8_t ssl)
{
    int sockfd_v6 = socket(AF_INET6, SOCK_STREAM, 0);
    int optval = 1;

    setsockopt(sockfd_v6, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sockfd_v6, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    int v6only = 1;

    setsockopt(sockfd_v6, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));
    struct sockaddr_in6 sa = {0};
    sa.sin6_family = AF_INET6;
    sa.sin6_port   = htons(port);
    if(addr == NULL)
        sa.sin6_addr = in6addr_any;
    else
        inet_pton(AF_INET6, addr, &sa.sin6_addr);

    bind(sockfd_v6, (struct sockaddr *)&sa, sizeof(sa));
    listen(sockfd_v6, backlog);

    ListenInfo *info = (ListenInfo*)malloc(sizeof(ListenInfo));
    info->fd = sockfd_v6;
    info->ssl = ssl;

    return info;
}
