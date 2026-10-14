#define _GNU_SOURCE
#include<unistd.h>
#include<assert.h>
#include<signal.h>
#include<execinfo.h>
#include<error.h>
#include<fcntl.h>


#include <sys/eventfd.h>
#include <sys/prctl.h>
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
    
    thread_pool *pool = initThreadPool(5, NULL);
   


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
