
#ifndef NETFREAMWORK_H
#define NETFREAMWORK_H

#include"datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

void initEnv(void);

ThreadPool* initThreadPool(int cnt, void* func);

void setThreadPoolCpuset(ThreadPool* pool, const cpu_set_t* set);

void destoryThreadPool(ThreadPool* pool);

void notifyThread(ThreadPool* pool, void* var);


int init_address(const char* addr, unsigned int port);
int init_address_v6(const char* addr, unsigned int port);



void netframework_loop(ThreadPool* pool, int* fds, int fd_cnt);



#ifdef __cplusplus
}
#endif
#endif //NETFREAMWORK_H
