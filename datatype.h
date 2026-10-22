

 #ifndef DATATYPE_H
 #define DATATYPE_H

#include<pthread.h>


typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;
typedef unsigned short   uint16_t;
typedef unsigned long   uint64_t;

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define OFFSET(type, field)   ((long) &((type *)0)->field)

#define lengthof(array) (sizeof (array) / sizeof ((array)[0]))


typedef struct ThreadInfo{
    pthread_t tid;
    int target;
    int pipefd[2];
    unsigned char flags;
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

struct connect_option{
    unsigned int fd;
}connect_option;

#endif // DATATYPE_H
