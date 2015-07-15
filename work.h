#ifndef FTP_WORK_H
#define FTP_WORK_H

#include "event.h"

//work进程和nobody进程进行通信所用命令宏


#define IPC_LISTEN_OPEN  1
#define IPC_ACCEPT		 2
#define IPC_CONNECT		 3


#define IPC_COMMAND_OK   5
#define IPC_COMMAND_BAD  6

//work进程向nobody进程发送命令                                                                                                                                                    
int IpcSendCommand(event_t *ptr,const char cmd);

//work进程接受来自nobody进程的命令结果                                                                                                                                            
int IpcRecvResult(event_t *ptr,char *res); 

int IpcRecvFd(event_t *ptr,int *fd);

int IpcSendDigit(event_t *ptr,int num);

void WorkInit(event_t *ptr);

void WorkHandle(event_t *ptr);

void NobodyInit(event_t *ptr);

void NobodyHandle(event_t *ptr);



#endif
