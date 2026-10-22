 #ifndef NETFREAM_RPC_H
 #define NETFREAM_RPC_H

typedef void (*cmd_hander) (void *thread_var, char *, int len);

typedef struct RpcCmd{
    unsigned int cmd_type;
    cmd_hander func;
 }RpcCmd;

#define MAX_CMD_CNT (256)
struct RpcCmd*  rpcCmdArray[MAX_CMD_CNT];


void registCmd(const unsigned int cmd, cmd_hander func);

void unregistCmd(const unsigned int cmd); 

unsigned int  parseRpcCmd(const char * msg, const unsigned int len, char **param, int *param_len);

 #endif
