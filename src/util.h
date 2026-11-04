
 #ifndef UTIL_H
 #define UTIL_H

typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;
typedef unsigned short   uint16_t;
typedef unsigned long   uint64_t;


#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

int lockSingleInstance(const char* lock_path);
void maskSignal();


#endif
