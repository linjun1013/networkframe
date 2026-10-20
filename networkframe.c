
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>


#include<fcntl.h>

#include"networkframe.h"

#include <sys/sdt.h>

void probeThread(void * arg)
{
    DTRACE_PROBE1(network, thread, arg);
}

static void* thread_work(void *arg)
{
    probeThread(arg);
    thread_info *info = (thread_info *)arg;
    int index = info ->target;
    printf("threads :%d\n", index);
    char name[16] = {0};
    snprintf(name, 16, "thread_%d",index);
    unsigned long tid = pthread_self();
    pthread_setname_np(tid, name);

    int pipefd = info->pipefd[0];
    void  (*fun_ptr)(thread_info * var, char *, int len) = info->thread_hander;

    char buff[16] = {0};
    char msg[1024] = {0};
    int cid = 0;
    int efd = epoll_create(16);
    struct epoll_event event;
    event.data.fd = pipefd;
    event.events = EPOLLIN;
    epoll_ctl(efd, EPOLL_CTL_ADD, pipefd, &event);
    struct epoll_event evs[16];

    while(1){
        int nfds = epoll_wait(efd, evs, 16, -1);
        for(int i = 0; i < nfds; i++)
        {
            if((evs[i].events & EPOLLIN && evs[i].data.fd != pipefd)){
                int fd = evs[i].data.fd;
                int len = 0, ret = 0;
                while((len < 1024) && (ret = read(fd, msg+len, 1024))){
                    len += ret;
                }

                if(unlikely(fun_ptr)){
                    fun_ptr((thread_info *)arg, msg, len);
                }else if(len){
                    write(fd, msg, sizeof(msg));
                }
                
                if(len == 0){
                
                }
            }else if(evs[i].events & EPOLLIN && evs[i].data.fd == pipefd){
                read(pipefd, buff, sizeof(buff));
                cid = atoi(buff);
                printf("get a new connect %d\n", cid);
                int flags = fcntl(cid, F_GETFL, 0);
                fcntl(cid, F_SETFL, flags | O_NONBLOCK);

                event.data.fd = cid;
                event.events = EPOLLIN|EPOLLOUT | EPOLLET | EPOLLRDHUP;
                epoll_ctl(efd, EPOLL_CTL_ADD, cid, &event);   
            }else if(evs[i].events & EPOLLOUT){
                int fd = evs[i].data.fd;
                write(fd, "111", 3);
            }else if(evs[i].events & EPOLLRDHUP){
                int fd = evs[i].data.fd;
                close(fd);
                printf("close  connect %d\n", fd);
            }
        }
    }

    printf("========exit ======= \n");
    free(arg);
    return NULL;
}



thread_pool* initThreadPool(int cnt,  void* func)
{
    if(cnt <= 0)
        return NULL;

    if(func == NULL)
        func = thread_work;

    thread_pool *pool = (thread_pool *)malloc(sizeof(thread_pool));
    pool->threads = (thread_info **)malloc(sizeof(void*)*cnt);

    for(int i = 0; i < cnt; i++)
    {
        thread_info *info = (thread_info *)malloc(sizeof(thread_info));
        info->target = i;
        pipe(info->pipefd);
        pool->threads[i] = info;
                                    
        printf("===== i:%d \n", i);
        pthread_create(&(info->tid), NULL, func, info);
        pthread_detach(info->tid);
    }

    return pool;
}

void destoryThreadPool(thread_pool *pool)
{
    if(pool == NULL || pool->threads == NULL)
        return;

    for(int i = 0; i< pool->size; i++){
        close(pool->threads[i]->pipefd[0]);
        close(pool->threads[i]->pipefd[1]);
        free(pool->threads[i]);
    }

    free(pool->threads);
    pool->threads = NULL;
}

void notifyThread(thread_pool *pool, void * var)
{
    if(pool == NULL || var == NULL)
        return;

    notify_info *info = (notify_info*)var;
    int index = info->index;
      
    char cid [16] = {0};
    snprintf(cid, 16, "%d", info->cid);
    write(pool->threads[index]->pipefd[1], cid, strlen(cid));
}

#define EPOLL_EVENT_CNT 16

void netframework_loop(thread_pool* pool, int *fds, int fd_cnt)
{
    if(pool == NULL || fds == NULL){
        printf("thread pool or fds is NULL \n");
        return;
    }

    int efd = epoll_create(256);
    struct epoll_event event;
    int pos = 0;
    for(; pos < fd_cnt; pos++){
        event.data.fd = fds[pos];
        event.events = EPOLLIN | EPOLLET;
        epoll_ctl(efd, EPOLL_CTL_ADD, fds[pos], &event);
     }
    struct epoll_event evs[EPOLL_EVENT_CNT];
    int client_sockfd = 0;
    static unsigned int c_ids = 0;
    int thread_cnt = pool->size;
    while(1){
        int nfds = epoll_wait(efd, evs, EPOLL_EVENT_CNT, -1);
        if(nfds > 0){
            int i = 0;
            for(; i < nfds; i++)
            {
                //if(evs[i].events & EPOLLIN && evs[i].data.fd  == fd){
                if(evs[i].events & EPOLLIN){
                    client_sockfd = accept(evs[i].data.fd, NULL, NULL);
                    char cid [16] = {0};
                    snprintf(cid, 16, "%d", client_sockfd);
                    int index = 0;
                    if(thread_cnt > 0)
                        index = c_ids++ % thread_cnt;

                    if(pool)
                        write(pool->threads[index]->pipefd[1], cid, strlen(cid));

                    printf("===> accept fd:%d  thread index:%d\n", client_sockfd, index);

                }
            }
        }           
    }    
}

int init_address(const char * addr, unsigned int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0)
        return 0;
    
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if(listen(sockfd, 256) != 0){
        printf("listen fd %d failed exit\n", sockfd);
        return 0;
    }

    return sockfd;
}


int init_address_v6(const char * addr, unsigned int port)
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
    sa.sin6_addr = in6addr_any;
    bind(sockfd_v6, (struct sockaddr *)&sa, sizeof(sa));
    listen(sockfd_v6, 256);
 
    return sockfd_v6;
}

