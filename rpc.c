#include<stdlib.h>
#include <string.h>
#include <stdio.h>
#include"rpc.h"

void registCmd(const unsigned int cmd, cmd_hander func)
{
   if(cmd == 0 || func == NULL)
        return;

   if(cmd > MAX_CMD_CNT || cmd == MAX_CMD_CNT)
        return;
    
    if(rpcCmdArray[cmd] != NULL){
        printf("cmd %d, have registed\n", cmd);
        return;
    }

    RpcCmd *rpc_cmd = (RpcCmd *)malloc(sizeof(RpcCmd));

    rpc_cmd->cmd_type = cmd;
    rpc_cmd->func = func;
    rpcCmdArray[cmd] = rpc_cmd;
}

void unregistCmd(const unsigned int cmd)
{
    if(cmd == 0 || cmd > MAX_CMD_CNT || cmd == MAX_CMD_CNT)
        return;

    if(rpcCmdArray[cmd]){
        free(rpcCmdArray[cmd]);
        rpcCmdArray[cmd] = NULL;
    }

}

// cmdtype + param len + param

unsigned int parseRpcCmd(const char * msg, const unsigned int len, char **param, int *param_len)
{
    if(msg == NULL || len == 0)
        return 0;
    
    int cmd_type = 0;
    char * pos = strchr(msg, '_');
    if(pos){
        int cmdlen = pos - msg;
        char cmd[cmdlen];
        strncpy(cmd, msg, len);
        cmd_type = atoi(cmd);
    }else{
        return 0;
    }

    if(*param != NULL){
        char *pos = strchr(pos, '_');
        *param = pos;
        int head_len = pos - msg;
        *param_len = len - head_len;     
    }

    return cmd_type;
}
