#include <stdlib.h>
#include <string.h>
#include"connhelper.h"

ConInfo* getConInfo(ConInfoList * list)
{
    if(list->head == NULL){
        ConInfo *con = (ConInfo *)malloc(sizeof(ConInfo));
        memset(con, 0, sizeof(ConInfo));
        list->head = con;
        list->tail = con;
        list->con_size = 1;
        return con;
    }else{
        ConInfo *con = list->head;
        list->head = con->next;
        if(list->tail == con)
            list->tail = NULL;
        list->con_size--;
        return con;
    }
}

void putConInfo(ConInfoList *list, ConInfo* con)
{
    if(con == NULL)
        return;
    if(list->con_size >= MAX_CON_INFO_SIZE){
        free(con);
        return;
    }else{
        memset(con, 0, sizeof(ConInfo));
        if(list->tail == NULL){
            list->tail = con;
        }else{
            list->tail->next = con;
            list->tail = con;
        }
    }
    if(list->head == NULL)
        list->head = con;
    list->con_size++;
}