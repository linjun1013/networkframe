
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include"datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

ThreadPool* initThreadPool(int cnt, void* func);

void setThreadPoolCpuset(ThreadPool* pool, const cpu_set_t* set);

void destoryThreadPool(ThreadPool* pool);

void notifyThread(ThreadPool* pool, void* var);

#ifdef __cplusplus
}
#endif
#endif //THREADPOOL_H
