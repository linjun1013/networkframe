#include<stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/sdt.h>

#include"rpc.h"

void registCmd(const unsigned int cmd, cmd_hander func)
{
   if(cmd == RPC_CMD_UNKNOW || func == NULL)
        return;

   if(cmd > RPC_CMD_MAX || cmd == RPC_CMD_MAX)
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
    if(cmd == RPC_CMD_UNKNOW || cmd > RPC_CMD_MAX || cmd == RPC_CMD_MAX)
        return;

    if(rpcCmdArray[cmd]){
        free(rpcCmdArray[cmd]);
        rpcCmdArray[cmd] = NULL;
    }

}

//cmdtype + param len + param

unsigned int parseRpcCmd(const char * msg, const unsigned int len, char **param, int *param_len)
{
    if(msg == NULL || len < RPC_CMD_HEAD_LEN)
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


static void probeExecRpcCmd(void * arg, int var)
{
     DTRACE_PROBE2(rpc, execRpcCmd, arg, var);
}

void* execRpcCmd(const int cmd, char* param, int param_len)
{
   probeExecRpcCmd(param, cmd);
   void* ret = NULL;
   if(cmd > RPC_CMD_UNKNOW && cmd < RPC_CMD_MAX){
        if(rpcCmdArray[cmd] && rpcCmdArray[cmd]->func){
            ret = rpcCmdArray[cmd]->func(param, param_len);
        }
   }

   return ret;
}

