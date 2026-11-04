
#ifndef NETFREAMWORK_H
#define NETFREAMWORK_H

#include"datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

void initEnv(const char *path);

void* net_thread_work(void *arg);

void netframework_loop(ThreadPool* pool, ListenInfo **infos, int fd_cnt);



#ifdef __cplusplus
}
#endif
#endif //NETFREAMWORK_H
