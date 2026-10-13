#define _GNU_SOURCE
#include<unistd.h>
#include<pthread.h>
#include<assert.h>
#include<signal.h>
#include<execinfo.h>
#include<error.h>
#include<fcntl.h>


#include <sys/eventfd.h>
#include <sys/prctl.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/ipc.h>




#include"util-hash.h"
#include"util.h"

#include<stdio.h>
#include<stdlib.h>

#include <immintrin.h>


#include <sys/user.h>
#include <errno.h>
#include "datatype.h"
#include "siphash.h"

#include <string.h>

#include <event2/http.h>
#include <event2/event.h>
#include <sys/wait.h>


void signal_handler(int sig)
{
    printf("cache signal %d, exit\n", sig);
    exit(0);
}

void signal_child(int sig){
    wait(NULL);
}

void* thread_work(void *arg)
{
    thread_info *info = (thread_info *)arg;
     int index = info ->target;
     printf("threads :%d\n", index);

     char name[16] = {0};
     snprintf(name, 16, "thread_%d",index);
 
     unsigned long tid = pthread_self();
    
     cpu_set_t cpuset;
     CPU_ZERO(&cpuset);
     CPU_SET(0, &cpuset); 
     CPU_SET(1, &cpuset); 
     pthread_setname_np(tid, name);
     pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);

    
     
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
                    while((len < 1024) && (ret = read(fd, msg+len, 1024)) > 0){ 
                        len += ret;
                    } 
                    printf("thread: %ld; off msg(%d)  %s==>\n",tid, len, msg);
                    if(unlikely(fun_ptr)){
                        fun_ptr((thread_info *)arg, msg, len);
                    }else if(len){
                        msg[0] = 'a';
                        write(fd, msg, sizeof(msg)); 
                   }
                   if(len == 0){
                        epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        printf("close fd ---> %d\n", fd);
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

int shm_id = 0;
char *shmptr;

int sockfd;


//char *un_path = "./unix_socket"; 




void root_handler(struct evhttp_request *req, void *arg) {
    struct evbuffer *buf = evbuffer_new();
    evbuffer_add_printf(buf, "Hello World\n");
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
}


void init_http_server(void)
{
    struct event_base *base = event_base_new();
    struct evhttp *http = evhttp_new(base);
    evhttp_set_cb(http, "/", root_handler, NULL);
    evhttp_bind_socket(http, "0.0.0.0", 8080);
    event_base_dispatch(base);

    evhttp_free(http);

    event_base_free(base);
    exit(1);
}








int  main(int argc, char **argv)
{
    if(!lockSingleInstance("./lock")){
        printf("already start \n");
        exit(-1);
    }
    prctl(PR_SET_NAME, "test", 0, 0, 0);

    nice(-1);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    thread_pool *pool = initThreadPool(5, thread_work);
   


   // pid_t pid = fork();
   // if(pid == 0){
   //     init_http_server();
   // }else{
   //     signal(SIGCHLD, signal_child);
   // }

    int fds[2];
    int fd_cnt = 0;
    fds[0] = init_address(NULL , 1234);
    fd_cnt++;

#ifdef CONFIG_IPV6_ENBALE
    fds[1] = init_address_v6(NULL, 1235);
    fd_cnt++;

#endif    

    start_netframework(pool, fds, fd_cnt);
    if(pool){
        destoryThreadPool(pool);
        free(pool);
    }
 
    return 0;
}
