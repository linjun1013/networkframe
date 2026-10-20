#define _GNU_SOURCE
#include<unistd.h>
#include<assert.h>
#include<signal.h>
#include<execinfo.h>
#include<error.h>


#include <sys/prctl.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/ipc.h>




#include"util-hash.h"
#include"util.h"

#include<stdio.h>

#include <immintrin.h>


#include <sys/user.h>
#include "datatype.h"
#include "siphash.h"

#include <string.h>




void signal_handler(int sig)
{
    printf("cache signal %d, exit\n", sig);
    exit(0);
}

void signal_child(int sig){
    wait(NULL);
}



int sockfd;





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
   

    int fds[2];
    int fd_cnt = 0;
    fds[0] = init_address(NULL , 1234);
    fd_cnt++;

#ifdef CONFIG_IPV6_ENBALE
    fds[1] = init_address_v6(NULL, 1235);
    fd_cnt++;

#endif    

    netframework_loop(pool, fds, fd_cnt);
    if(pool){
        destoryThreadPool(pool);
        free(pool);
    }
 
    return 0;
}
