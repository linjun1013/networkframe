

 #ifndef DATATYPE_H
 #define DATATYPE_H

#include<pthread.h>
#include"util-hash.h"

#ifdef __cplusplus
extern "C" {
#endif


#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define OFFSET(type, field)   ((long) &((type *)0)->field)

#define lengthof(array) (sizeof (array) / sizeof ((array)[0]))


typedef struct ThreadInfo{
    pthread_t tid;
    int target;
    int pipefd[2];
    unsigned char flags;
    HashTable* fd_bufhash;
    void*   private;
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

typedef struct connect_option{
    unsigned int fd;
}connect_option;


typedef struct FdBuff{
    int fd;
    char fixbuf[1024];
    void *extbuf;
    int ext_len;
}FdBuff;


#ifdef __cplusplus
}
#endif
#endif // DATATYPE_H
