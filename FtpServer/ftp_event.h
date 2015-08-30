#ifndef FTP_EVENT_H
#define FTP_EVENT_H

#include "common.h"
# define MAX_LEN 1024

typedef struct
{
    int fd;

    unsigned read : 1;
    unsigned write : 1;

    int (*read_handler)(void*);
    int (*write_handler)(void*);

}ftp_event_t;

typedef struct
{
	char command[MAX_LEN];		//client发来的FTP指令
	char com[MAX_LEN];			//命令
	char args[MAX_LEN];			//参数

	int connfd;			    	//client与server的控制连接fd
	int listenfd;				//server监听fd,pasv模式下使用

	int nobodyfd;

	int login;					//登陆状态
	char username[100];			//登录名
	uid_t useruid;				//登陆id

	int pasv;					//是否打开pasv模式
	int port;					//是否打开port模式

	int transmode;				//传输模式,0为ASCII码模式，1为binary模式

	size_t restart_pos;		    //断点重传的起点

	struct sockaddr_in *addr;	//client地址和端口号，port模式使用
}ftp_connection_t;

#endif
