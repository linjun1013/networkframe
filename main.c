#define _GNU_SOURCE
#include<unistd.h>

#include<error.h>


#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/prctl.h>

#include"util.h"

#include<stdio.h>

#include <sys/user.h>
#include "datatype.h"

#include <string.h>


#include "networkframe.h"
#include "rpc.h"


int main(void)
{
    prctl(PR_SET_NAME, "net_loop_work", 0, 0, 0);
    nice(-1);

    initEnv(NULL);
    ThreadPool *pool = initThreadPool(5, NULL);

    ListenInfo* fds[2];
    int fd_cnt = 0;
    fds[0] = init_address(NULL , 1234, 256 ,0);
    fd_cnt++;

#ifdef CONFIG_IPV6_ENBALE
    fds[1] = init_address_v6(NULL, 1235, 256, 0);
    fd_cnt++;
#endif 

    netframework_loop(pool, fds, fd_cnt);
    if(pool){
        destoryThreadPool(pool);
        free(pool);
    }
 
    return 0;
}
