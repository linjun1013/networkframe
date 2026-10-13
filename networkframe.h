
#ifndef NETFREAMWORK_H
#define NETFREAMWORK_H

#include"datatype.h"



thread_pool* initThreadPool(int cnt, void * func);

void destoryThreadPool(thread_pool *pool);

void notifyThread(thread_pool *pool, void * var);


int init_address(const char * addr, unsigned int port);
int init_address_v6(const char * addr, unsigned int port);

void start_netframework(thread_pool* pool, int *fds, int fd_cnt);


#endif
