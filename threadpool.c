#define _GNU_SOURCE
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <pthread.h>

#include<fcntl.h>
#include<errno.h>

#include"threadpool.h"

void setThreadPoolCpuset(ThreadPool* pool, const cpu_set_t* set)
{
    if(pool == NULL || set == NULL)
        return;

    for(int i = 0; i< pool->size; i++)
    {
        sched_setaffinity(pool->threads[i]->tid, sizeof(*set), set);
    }
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

ThreadPool* initThreadPool(int cnt,  void* func)
{
    if(cnt <= 0 || func == NULL){
        printf("thread pool cnt is zero or func is NULL\n");
        return NULL;
    }


    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pool->threads = (ThreadInfo **)malloc(sizeof(void*)*cnt);
    pool->size = cnt;

    for(int i = 0; i < cnt; i++)
    {
        ThreadInfo *info = (ThreadInfo *)malloc(sizeof(ThreadInfo));
        memset(info, 0, sizeof(ThreadInfo));
        info->target = i;
        pipe(info->pipefd);

        info->bufhash = HashTableInit(16, bufHash, bufHashCompare, bufHashFree);

        pool->threads[i] = info;

        printf("===== i:%d \n", i);
        pthread_create(&(info->tid), NULL, func, info);
        pthread_detach(info->tid);
    }

    return pool;
}

void destoryThreadPool(ThreadPool *pool)
{
    if(pool == NULL || pool->threads == NULL)
        return;

    for(int i = 0; i< pool->size; i++){
        close(pool->threads[i]->pipefd[0]);
        close(pool->threads[i]->pipefd[1]);
        HashTableFree(pool->threads[i]->bufhash);
        free(pool->threads[i]);
    }

    free(pool->threads);
    pool->threads = NULL;
    pool->size = 0;
}

void notifyThread(ThreadPool *pool, void * var)
{
    if(pool == NULL || var == NULL)
        return;

    NotifyInfo *info = (NotifyInfo*)var;
    int index = info->index;
      
    char cid [16] = {0};
    snprintf(cid, 16, "%d", info->cid);
    write(pool->threads[index]->pipefd[1], cid, strlen(cid));
}


