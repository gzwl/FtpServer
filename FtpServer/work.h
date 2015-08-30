#ifndef FTP_WORK_H
#define FTP_WORK_H

#include "ftp_event.h"

//work进程和nobody进程进行通信所用命令宏


#define IPC_LISTEN_OPEN  1
#define IPC_ACCEPT		 2
#define IPC_CONNECT		 3


#define IPC_COMMAND_OK   5
#define IPC_COMMAND_BAD  6





void WorkInit(ftp_event_t *ptr);

void NobodyInit(ftp_event_t *ptr);

void NobodyHandle(ftp_event_t *ptr);



#endif
