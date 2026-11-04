 #ifndef NETFREAM_RPC_H
 #define NETFREAM_RPC_H

 #include<jansson.h>

typedef void* (*cmd_hander) (void *);

typedef struct RpcCmd{
    unsigned int cmd_type;
    cmd_hander func;
 }RpcCmd;

#define RPC_CMD_HEAD_LEN (4)

enum{
    RPC_CMD_UNKNOW=0,
    RPC_CMD_TEST,
    RPC_CMD_MAX
};

struct RpcCmd*  rpcCmdArray[RPC_CMD_MAX];


void registCmd(const unsigned int cmd, cmd_hander func);

void unregistCmd(const unsigned int cmd); 

unsigned int parseRpcCmd(const char * msg, const unsigned int len, json_t **param);


void* execRpcCmd(const int cmd, const json_t* param);

 #endif
