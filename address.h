
#ifndef ADDRESS_H
#define ADDRESS_H

#include"datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

ListenInfo* init_address(const char* addr, unsigned int port, int backlog, uint8_t ssl);
ListenInfo* init_address_v6(const char* addr, unsigned int port, int backlog, uint8_t ssl);

#ifdef __cplusplus
}
#endif
#endif //THREADPOOL_H
