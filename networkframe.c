#define _GNU_SOURCE
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>


#include<fcntl.h>
#include<errno.h>
#include<signal.h>


#ifdef CONFIG_SSL
#include <openssl/err.h>
#endif

#include"networkframe.h"
#include"rpc.h"
#include"connhelper.h"

#include <sys/sdt.h>

#ifdef CONFIG_SSL
SSL_CTX *ssl_ctx;
#endif

static void probeThread(void * arg, int var)
{
    DTRACE_PROBE2(network, thread, arg, var);   
}

static void peobeRead(void *arg)
{
    DTRACE_PROBE1(network, read, arg);
}

static void peobeProxyRead(int fd, int proxy_fd)
{
    DTRACE_PROBE2(network, proxyread, fd, proxy_fd);
}


#define ADD_READ_LEN(len, ret) \
if(ret > 0 ) \
    len += ret; \
else \
    break; \


static void add_accept(int pipefd, int efd, ThreadInfo *info)
{
    ConInfo *con = getConInfo(&(info->con_list));    
    unsigned long cip_fd = 0;
    read(pipefd, &cip_fd, sizeof(cip_fd));
    int cip = cip_fd >> 32;
    unsigned int cid = (cip_fd & 0x00000000ffffffff);
    printf("==> get a new connect %d pipe:%d\n", cid, pipefd);
    int flags = fcntl(cid, F_GETFL, 0);
    fcntl(cid, F_SETFL, flags | O_NONBLOCK);

    con->cip = cip;
    con->fd = cid;
    struct epoll_event event;

#ifdef CONFIG_SSL
    uint8_t ssl_flag = 0;
    read(pipefd, &ssl_flag, sizeof(ssl_flag));
    if(ssl_flag){
        unsigned long ssl_ptr = 0;
        read(pipefd, &ssl_ptr, sizeof(ssl_ptr));
        con->ssl = (SSL*)ssl_ptr;
    }
#elif CONFIG_PROXY
    int proxy_fd = 0;
    read(pipefd, &proxy_fd, sizeof(proxy_fd));
    if(proxy_fd){
        int flags = fcntl(proxy_fd, F_GETFL, 0);
        fcntl(proxy_fd, F_SETFL, flags | O_NONBLOCK);
        con->proxy = (ProxyInfo *)malloc(sizeof(ProxyInfo));
        pipe2(con->proxy->pipefd, O_NONBLOCK);
        con->proxy->proxy_fd = proxy_fd;
        con->proxy->con = con;
        event.data.ptr = con->proxy;
        event.events = EPOLLOUT | EPOLLET;
        if(epoll_ctl(efd, EPOLL_CTL_ADD, proxy_fd, &event))
            printf("epoll_ctl add proxy failed %d efd:%d\n", cid, efd);
    }
#endif

    event.data.ptr = con;
    event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, cid, &event))
        printf("epoll_ctl add failed %d efd:%d\n", cid, efd);

#ifndef CONFIG_PROXY
    FdBuff *buf = (FdBuff*)malloc(sizeof(FdBuff));
    memset(buf, 0, sizeof(FdBuff));
    buf->con.fd = cid;
    buf->con.cip = cip;
    HashTableAdd(info->bufhash, buf, sizeof(buf));
#endif
}

#ifdef CONFIG_PROXY
static void freeProxy(ConInfo *con)
{
    if(con->proxy == NULL)
        return;
    close(con->proxy->pipefd[0]);
    close(con->proxy->pipefd[1]);
    con->proxy = NULL;
}
#endif


#ifdef CONFIG_PROXY
static void proxy_recv_msg(int efd, ConInfo *con, ThreadInfo *info)
{
    ProxyInfo *proxy = con->proxy;
    if(proxy == NULL)
        return;

    peobeProxyRead(con->fd, proxy->proxy_fd);

    int fd = con->fd;
    uint8_t proxy_flag = (proxy != NULL)? 1 : 0;
    int cnt = 0;
    while (proxy_flag) {
        int ret = splice(fd, NULL, proxy->pipefd[1], NULL, 32768, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                printf("splice read");
                break;
            }
        } else if (ret == 0) {
            break;
        }else{
            cnt += ret;
        }
    }
    int written = 0;
    while (proxy_flag && cnt)
    {
        if(written < cnt) {
            ssize_t w = splice(proxy->pipefd[0], NULL, proxy->proxy_fd, NULL, cnt - written, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
            if (w <=  0) {
                printf("splice write:%d error\n", w);
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                   freeProxy(con);
                   epoll_ctl(efd, EPOLL_CTL_DEL, proxy->proxy_fd, NULL);
                   epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                   putConInfo(&(info->con_list), con);
                   close(fd);
                   break;
                } else {
                   break;
                } 
            }else{
                written += w;
            }
        }else{
            break;
        }   
    }
}
#endif

static void recv_msg(int efd, ConInfo *con, ThreadInfo *info)
{
    int fd = con->fd;
    int len = 0, ret = 0;
    FdBuff *fdbuf = (FdBuff *)HashTableLookup(info->bufhash, con, sizeof(con));
    if(unlikely(fdbuf == NULL)){
        printf("get fd buf failed %d\n", fd);
        return;
    }

    char *msg = fdbuf->fixbuf;
    int max_msg_len = 512;
    if(fdbuf->extbuf){
        msg = fdbuf->extbuf;
        max_msg_len = fdbuf->ext_len;
    }
#ifdef CONFIG_SSL
    if(con->ssl){
        while (len < max_msg_len &&(ret = SSL_read(con->ssl, msg+len, max_msg_len)) > 0){
            ADD_READ_LEN(len, ret)
        }
    }else{
        while((len < max_msg_len) && (ret = read(fd, msg+len, max_msg_len))) {
            ADD_READ_LEN(len, ret)
        }
    }
#else
    if(fd > 0){
        while((len < max_msg_len) && (ret = read(fd, msg+len, max_msg_len))) {
            ADD_READ_LEN(len, ret)
        }
    }
#endif
    if(len < max_msg_len)
        msg[len] = 0;
    else
        msg[max_msg_len - 1] = 0;

    void  (*fun_ptr)(ThreadInfo * var, char *, int len) = info->thread_hander;
    peobeRead(msg);
    printf(" ==> get a new read from  %d  len:%d , msg:%p\n", fd, len, msg);
    if(unlikely(fun_ptr)){
        fun_ptr(info, msg, len);
    }else if(len){
        json_t * param = NULL;
        int param_len;
        unsigned int cmd_type = parseRpcCmd((const char *)msg, len, &param);
        execRpcCmd(cmd_type, param);
        if(param)
            json_decref(param);

        write(fd, msg, len);
    }
    if((ret == 0) || (ret < 0 && errno != EAGAIN)){
        epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
#ifdef CONFIG_SSL
        if(con->ssl)
            SSL_free(con->ssl);
#endif
        HashTableRemove(info->bufhash, con, sizeof(con));
        putConInfo(&(info->con_list), con);
        close(fd);
    }       
}


void* net_thread_work(void *arg)
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
#ifdef CONFIG_PROXY
                proxy_recv_msg(efd, con, info);
#else
                recv_msg(efd, con, info);
#endif
            }else if((evs[i].events & EPOLLIN) && evs[i].data.fd == pipefd){
                add_accept( pipefd, efd, info);
            }else if(evs[i].events & EPOLLOUT){

#ifdef CONFIG_PROXY
                ProxyInfo *proxy = (ProxyInfo*)evs[i].data.ptr;
                int proxy_fd = proxy->proxy_fd;
                int cfd = proxy->con->fd;
                int len = splice(proxy_fd, NULL, proxy->pipefd[1], NULL, 32768, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
                if(len <= 0) {
                    if(errno != EAGAIN && errno != EWOULDBLOCK){
                        close(proxy_fd);
                        epoll_ctl(efd, EPOLL_CTL_DEL, proxy_fd, NULL);
                    }
                }else{
                    splice(proxy->pipefd[0], NULL, cfd, NULL, 32768, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
                }
#endif
            }else if(evs[i].events & (EPOLLHUP | EPOLLERR) && (evs[i].data.fd != pipefd)){
                ConInfo *con = (ConInfo*)evs[i].data.ptr;
                int fd = con->fd;
                epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
#ifdef CONFIG_SSL
                if(con->ssl)
                    SSL_free(con->ssl);
#elif  CONFIG_PROXY
                freeProxy(con);
#endif
#ifndef CONFIG_SSL
                HashTableRemove(info->bufhash, con, sizeof(con));
#endif
                putConInfo(&(info->con_list), con);
            }
        }
    }

    printf("========exit ======= \n");
    free(arg);
    return NULL;
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

    free(buf);
}

static void signal_handler(int sig)
{
    printf("cache signal %d, exit\n", sig);
    exit(0);
}

void initEnv(const char *path)
{

#ifdef CONFIG_INSTANCE
    if(!lockSingleInstance("./lock")){
        printf("already start \n");
        exit(-1);
    }
#endif

#ifdef CONFIG_SSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    ssl_ctx = SSL_CTX_new(SSLv23_server_method());
    if(path == NULL){
        if (SSL_CTX_use_certificate_chain_file(ssl_ctx, "./server.crt") <= 0)
        {
            printf("ssl load crt failed\n");
        }
        if (SSL_CTX_use_PrivateKey_file(ssl_ctx, "./server.key", SSL_FILETYPE_PEM) <= 0){
            printf("ssl load key failed\n");
        }
    }else{
        char crt[256] = {0};
        char key[256] = {0};
        snprintf(crt, 256, "%s/%s",path, "server.crt");
        if (SSL_CTX_use_certificate_chain_file(ssl_ctx, crt) <= 0)
        {
            printf("ssl load crt failed\n");
        }
        snprintf(key, 256, "%s/%s",path, "server.key");
        if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key, SSL_FILETYPE_PEM) <= 0){
            printf("ssl load key failed\n");
        }
    }

    if(!SSL_CTX_check_private_key(ssl_ctx))
    {
        printf("load ssl crt, key failed\n");
    }

#endif

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
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
        event.events = EPOLLIN | EPOLLHUP | EPOLLERR;
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

                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    getpeername(client_sockfd, (struct sockaddr *)&client_addr, &addr_len);

                    unsigned long ip_fd = ((unsigned long)(client_addr.sin_addr.s_addr) << 32);
                    ip_fd = ip_fd | client_sockfd;
#ifdef CONFIG_SSL
                    uint8_t ssl_flag = 0;
                    if(list_info->ssl){
                        ssl_flag = 1;
                    }

                    if(ssl_flag){
                        SSL *ssl = SSL_new(ssl_ctx);
                        SSL_set_fd(ssl, client_sockfd);
                        int ret = SSL_accept(ssl);
                        if(ret <= 0){
                            printf("SSL_accept failed :%d", ret);
                            SSL_free(ssl);
                            continue;
                        }
                        unsigned long ssl_ptr = (unsigned long)ssl;
                        write(pool->threads[index]->pipefd[1], &ip_fd, sizeof(ip_fd));
                        write(pool->threads[index]->pipefd[1], &ssl_flag, sizeof(ssl_flag));
                        write(pool->threads[index]->pipefd[1], &ssl_ptr, sizeof(ssl_ptr));
                    }else{
                        write(pool->threads[index]->pipefd[1], &ip_fd, sizeof(ip_fd));
                        write(pool->threads[index]->pipefd[1], &ssl_flag, sizeof(ssl_flag));
                    }
#else
                    write(pool->threads[index]->pipefd[1], &ip_fd, sizeof(ip_fd));
#ifdef CONFIG_PROXY
                    int proxy_fd = 0;
                    if(list_info->proxy_ip){
                        proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
                        struct sockaddr_in sin;
                        sin.sin_family = AF_INET;
                        sin.sin_port = htons(list_info->proxy_port);
                        sin.sin_addr.s_addr = htonl(list_info->proxy_ip);
                        connect(proxy_fd, (struct sockaddr *)&sin, sizeof(sin));
                    }
                    write(pool->threads[index]->pipefd[1], &proxy_fd, sizeof(proxy_fd));
#endif
#endif

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


