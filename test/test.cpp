#include<stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include<fcntl.h>


#ifdef CONFIG_SSL
#include <openssl/ssl.h>
#endif
void proxy()
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1237);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(listen(listen_fd, 256)  != 0){
        printf("listen listen_fd %d failed exit\n", listen_fd);
        return;
    }

    int efd = epoll_create(256);
    struct epoll_event event;
    event.data.fd = listen_fd;
    event.events = EPOLLIN | EPOLLHUP | EPOLLERR;
    epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &event);
    struct epoll_event evs[16];
    char msg[512] = {0};
    while(1)
    {
        int nfds = epoll_wait(efd, evs, 16, -1);
        for(int i = 0; i < nfds; i++){
            if(evs[i].events & EPOLLIN){
                int fd = evs[i].data.fd;
                if(fd == listen_fd){
                    int cfd = accept(listen_fd, NULL, NULL);
                    int flags = fcntl(cfd, F_GETFL, 0);
                    fcntl(cfd, F_SETFL, flags | O_NONBLOCK);
                    event.data.fd = cfd;
                    event.events = EPOLLIN | EPOLLHUP | EPOLLERR;
                    epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &event);
                }else{
                    int len = read(fd, msg, 512);
                    if(len < 512)
                        msg[len] = 0;
                    else 
                        msg[511] = 0;
                }
            }
        }
    }

}
int main(void)
{

#ifdef CONFIG_SSL
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    const SSL_METHOD *meth = SSLv23_client_method();
    SSL_CTX* ctx = SSL_CTX_new(meth);
#endif
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);
    int fd[1000] = {0};
    int ret = 0;
    for(int i = 0; i < 200; i++)
    {   
        fd[i] = socket(AF_INET, SOCK_STREAM, 0);
        if(connect(fd[i], (sockaddr*)&sin, sizeof(sin)) == -1 )
        {
            printf("fd test --- :%d failed \n", i);
        }else{
#ifdef CONFIG_SSL
            SSL*ssl = SSL_new(ctx);
            SSL_set_fd(ssl, fd[i]);
            SSL_connect(ssl);
            SSL_write(ssl, "111", 3);
            SSL_free(ssl);
#else
            write(fd[i], "111", 3);
#endif
            ret++;
        }
        
    
    }

#ifdef CONFIG_SSL    
    SSL_CTX_free(ctx);
#endif

    printf("fd test cnt:%d\n", ret);
    sleep(10);


    for(int i = 0; i < ret; i++)
    {
        shutdown(fd[i], SHUT_RDWR);
        close(fd[i]);
    }

    while(1);

}
