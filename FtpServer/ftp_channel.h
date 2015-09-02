#ifndef FTP_CHANNEL_H
#define FTP_CHANNEL_H

#include <stddef.h>

//进程间通信所用命令宏

#define IPC_LISTEN_OPEN  1000
#define IPC_ACCEPT		 1001
#define IPC_CONNECT		 1002


#define FTP_IPC_OK   1003
#define FTP_IPC_BAD  1004

typedef struct{
    int message;
	int slot;
	int fd;
}ftp_channel_t;

int ftp_ipc_send_msg(int sockfd,int msg,int fd);
int ftp_ipc_recv_msg(int sockfd,int* msg,int* fd);

#endif
