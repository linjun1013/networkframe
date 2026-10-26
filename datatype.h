

 #ifndef DATATYPE_H
 #define DATATYPE_H

#include<pthread.h>

#ifdef CONFIG_SSL
#include <openssl/ssl.h>
#endif

#include"util-hash.h"

#ifdef __cplusplus
extern "C" {
#endif


#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define OFFSET(type, field)   ((long) &((type *)0)->field)

#define LENGTHOF(array) (sizeof (array) / sizeof ((array)[0]))


typedef struct ThreadInfo{
    pthread_t tid;
    int target;
    int pipefd[2];
    unsigned char flags;
    HashTable* bufhash;
    void* data;
    void (*thread_hander) (struct ThreadInfo *thread_var, char *, int len);
} ThreadInfo;


typedef struct ThreadPool{
    ThreadInfo **threads;
    int size;
}ThreadPool;

typedef struct NotifyInfo{
    int index;
    unsigned int cid;
}NotifyInfo;

typedef struct ListenInfo{
    int fd;
    uint8_t ssl;
}ListenInfo;

typedef struct ConInfo{
    unsigned int cip;
    unsigned int fd;
#ifdef CONFIG_SSL
    SSL* ssl;
#endif
}ConInfo;



typedef struct FdBuff{
    ConInfo con;
    char fixbuf[512];
    void *extbuf;
    int ext_len;
}FdBuff;


#ifdef __cplusplus
}
#endif
#endif // DATATYPE_H
