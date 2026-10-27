
#ifndef NETFREAMWORK_H
#define NETFREAMWORK_H

#include"datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

void initEnv(const char *path);

ThreadPool* initThreadPool(int cnt, void* func);

void setThreadPoolCpuset(ThreadPool* pool, const cpu_set_t* set);

void destoryThreadPool(ThreadPool* pool);

void notifyThread(ThreadPool* pool, void* var);


ListenInfo* init_address(const char* addr, unsigned int port, int backlog, uint8_t ssl);
ListenInfo* init_address_v6(const char* addr, unsigned int port,int backlog, uint8_t ssl);


void netframework_loop(ThreadPool* pool, ListenInfo **infos, int fd_cnt);



#ifdef __cplusplus
}
#endif
#endif //NETFREAMWORK_H
