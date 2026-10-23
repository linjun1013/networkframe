
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>


#include<fcntl.h>
#include<errno.h>
#include<signal.h>


#include"networkframe.h"
#include"rpc.h"

#include <sys/sdt.h>




void probeThread(void * arg, int var)
{
    DTRACE_PROBE2(network, thread, arg, var);   
}

void peobeRead(void *arg)
{
    DTRACE_PROBE1(network, read, arg);
}
    
static void* thread_work(void *arg)
{
    ThreadInfo *info = (ThreadInfo *)arg;
    int index = info ->target;
    probeThread(info, index);
    printf("threads :%d\n", index);
    char name[16] = {0};
    snprintf(name, 16, "thread_%d",index);
    unsigned long tid = pthread_self();
    pthread_setname_np(tid, name);

    int pipefd = info->pipefd[0];
    void  (*fun_ptr)(ThreadInfo * var, char *, int len) = info->thread_hander;

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
            if(((evs[i].events & EPOLLIN) && evs[i].data.fd != pipefd)){
                ConInfo *con = (ConInfo*)evs[i].data.ptr;
                int fd = con->fd;
                int len = 0, ret = 0;
                
                FdBuff *fdbuf = (FdBuff *)HashTableLookup(info->bufhash, con, sizeof(con));
                if(fdbuf == NULL){
                    printf("get fd buf failed %d\n", fd);
                    continue;
                }

                char *msg = fdbuf->fixbuf;
                int max_msg_len = 512;
                if(fdbuf->extbuf){
                    msg = fdbuf->extbuf;
                    max_msg_len = fdbuf->ext_len;
                }
                                    
                while((len < max_msg_len) && (ret = read(fd, msg+len, max_msg_len))) {
                    if(ret > 0)
                        len += ret;
                    else
                        break;
                }

                if(len < max_msg_len)
                    msg[len] = 0;
                else
                    msg[ max_msg_len - 1] = 0;
                
                peobeRead(msg);
                printf(" ==> get a new read %d  len:%d , msg:%p\n", fd, len, msg);
                if(unlikely(fun_ptr)){
                    fun_ptr((ThreadInfo *)arg, msg, len);
                }else if(len){
                    char * param;
                    int param_len;
                     unsigned int cmd_type = parseRpcCmd((const char *)msg, len, &param, &param_len);
                     execRpcCmd(cmd_type, param, param_len);
                     write(fd, msg, len);
                }
                
                if((ret == 0) || (ret < 0 && errno != EAGAIN)){
                    epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    free(con);
                }
            }else if((evs[i].events & EPOLLIN) && evs[i].data.fd == pipefd){
                unsigned long cip_fd = 0;
                read(pipefd, &cip_fd, sizeof(cip_fd));
                int cip = cip_fd >> 32;
                unsigned int cid = (cip_fd & 0x00000000ffffffff);
                printf("==> get a new connect %d pipe:%d\n", cid, pipefd);
                int flags = fcntl(cid, F_GETFL, 0);
                fcntl(cid, F_SETFL, flags | O_NONBLOCK);
                
                ConInfo *con = (ConInfo *)malloc(sizeof(ConInfo));
                con->cip = cip;
                con->fd = cid;

                struct epoll_event event;
                event.data.ptr = con;
                event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
                if(epoll_ctl(efd, EPOLL_CTL_ADD, cid, &event))
                    printf("epoll_ctl add failed %d efd:%d\n", cid, efd);  
            }else if(evs[i].events & EPOLLOUT){
                //int fd = evs[i].data.fd;
                //write(fd, "111", 3);
            }else if(evs[i].events & (EPOLLHUP | EPOLLERR) && (evs[i].data.fd != pipefd)){
                ConInfo *con = (ConInfo*)evs[i].data.ptr;
                int fd = con->fd;
                epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                free(con);
            }
        }
    }

    printf("========exit ======= \n");
    free(arg);
    return NULL;
}


void setThreadPoolCpuset(ThreadPool* pool, const cpu_set_t* set)
{
    if(pool == NULL || set == NULL)
        return;

    for(int i = 0; i< pool->size; i++)
    {
        sched_setaffinity(pool->threads[i]->tid, sizeof(*set), &set);    
    }
}

static uint32_t bufHash(HashTable *ht, void *data, uint16_t datalen)
{
    FdBuff *buf = (FdBuff*)data;

    unsigned long long hash = ((unsigned long long)buf->con.cip << 32) | buf->con.fd;
    
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;
    hash = (unsigned int)(hash ^ (hash >> 32));
    hash %= ht->array_size;

    return hash;
}

static char bufHashCompare(void *data1, uint16_t len1, void *data2, uint16_t len2)
{
    ConInfo *con1 = &(((FdBuff *)data1)->con);
    ConInfo *con2 = (ConInfo*)data2;

    if(con1->cip == con2->cip && con1->fd == con2->fd)
        return 1;

    return 0;   
}

static void bufHashFree(void *data)
{
    if(data == NULL)
        return;
    FdBuff *buf = (FdBuff*)data;
    if(buf->extbuf)
        free(buf->extbuf);
    buf->extbuf = NULL;
    buf->ext_len = 0;
}

static void signal_handler(int sig)
{
    printf("cache signal %d, exit\n", sig);
    exit(0);
}

void initEnv(void)
{

#ifdef CONFIG_INSTANCE
    if(!lockSingleInstance("./lock")){
        printf("already start \n");
        exit(-1);
    }
#endif

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
}

ThreadPool* initThreadPool(int cnt,  void* func)
{
    if(cnt <= 0)
        return NULL;

    if(func == NULL)
        func = thread_work;

    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pool->threads = (ThreadInfo **)malloc(sizeof(void*)*cnt);

    for(int i = 0; i < cnt; i++)
    {
        ThreadInfo *info = (ThreadInfo *)malloc(sizeof(ThreadInfo));
        info->target = i;
        pipe(info->pipefd);

        info->bufhash = HashTableInit(16, bufHash, bufHashCompare, bufHashFree);
        pool->threads[i] = info;

        printf("===== i:%d \n", i);
        pthread_create(&(info->tid), NULL, func, info);
        pthread_detach(info->tid);
    }

    return pool;
}

void destoryThreadPool(ThreadPool *pool)
{
    if(pool == NULL || pool->threads == NULL)
        return;

    for(int i = 0; i< pool->size; i++){
        close(pool->threads[i]->pipefd[0]);
        close(pool->threads[i]->pipefd[1]);
        HashTableFree(pool->threads[i]->bufhash);
        free(pool->threads[i]);
    }

    free(pool->threads);
    pool->threads = NULL;
}

void notifyThread(ThreadPool *pool, void * var)
{
    if(pool == NULL || var == NULL)
        return;

    NotifyInfo *info = (NotifyInfo*)var;
    int index = info->index;
      
    char cid [16] = {0};
    snprintf(cid, 16, "%d", info->cid);
    write(pool->threads[index]->pipefd[1], cid, strlen(cid));
}

#define EPOLL_EVENT_CNT 16

void netframework_loop(ThreadPool* pool, ListenInfo **infos, int fd_cnt)
{
    if(pool == NULL || infos == NULL){
        printf("thread pool or fds is NULL \n");
        return;
    }

    int efd = epoll_create(256);
    struct epoll_event event;

    for(int pos = 0; pos < fd_cnt; pos++){
        ListenInfo *listen_info = infos[pos];
        event.data.ptr = listen_info;
        event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
        epoll_ctl(efd, EPOLL_CTL_ADD, listen_info->fd, &event);
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
                if(evs[i].events & EPOLLIN){
                    ListenInfo *list_info = (ListenInfo *)evs[i].data.ptr;
                    client_sockfd = accept(list_info->fd, NULL, NULL);
                    int index = 0;
                    if(likely(thread_cnt > 0))
                        index = c_ids++ % thread_cnt;

                    ThreadInfo *info = pool->threads[index];
                    FdBuff *buf = (FdBuff*)malloc(sizeof(FdBuff));
                    buf->con.fd = client_sockfd;

                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    getpeername(client_sockfd, (struct sockaddr *)&client_addr, &addr_len);
                    buf->con.cip = client_addr.sin_addr.s_addr;
                    HashTableAdd(info->bufhash, buf, sizeof(buf));

                    unsigned long ip_fd = ((unsigned long)buf->con.cip << 32);
                    ip_fd = ip_fd | client_sockfd;

                    write(pool->threads[index]->pipefd[1], &ip_fd, sizeof(ip_fd));

                    uint8_t ssl_flag = 0;
                    if(list_info->ssl){
                        ssl_flag = 1;
                    }
                    write(pool->threads[index]->pipefd[1], &ssl_flag, sizeof(ssl_flag));

#ifdef NETWORK_DEBUG
                    printf("===> accept fd:%d  thread index:%d, ip_fd:%lu\n", client_sockfd, index, ip_fd);
#endif
                }else if(evs[i].events & (EPOLLHUP | EPOLLERR)){
                    ListenInfo *info = (ListenInfo *)evs[i].data.ptr;
                    epoll_ctl(efd, EPOLL_CTL_DEL, info->fd, NULL);
                    free(info);
                }
            }
        }
    }
}

ListenInfo * init_address(const char* addr, unsigned int port, uint8_t ssl)
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
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if(listen(sockfd, 256) != 0){
        printf("listen fd %d failed exit\n", sockfd);
        return NULL;
    }

    ListenInfo *info = (ListenInfo*)malloc(sizeof(ListenInfo));
    info->fd = sockfd;
    info->ssl = ssl;

    return info;
}


ListenInfo* init_address_v6(const char * addr, unsigned int port, uint8_t ssl)
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

    ListenInfo *info = (ListenInfo*)malloc(sizeof(ListenInfo));
    info->fd = sockfd_v6;
    info->ssl = ssl;
 
    return info;
}

