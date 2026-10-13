

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


typedef struct thread_info{
    pthread_t tid;
    int target;
    int pipefd[2];
    unsigned char flags;
    void (*thread_hander) (struct thread_info *thread_var, char *, int len);
} thread_info;


typedef struct thread_pool{
    thread_info **threads;
    int size;
}thread_pool;

typedef struct notify_info{
    int index;
    unsigned int cid;
}notify_info;


#endif // DATATYPE_H
