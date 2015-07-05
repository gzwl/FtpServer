#ifndef EVENT_H
#define EVENT_H

# define MAX_LEN 1024
typedef struct
{
	char command[MAX_LEN];	//client发来的FTP指令
	char com[MAX_LEN];		//命令
	char args[MAX_LEN];		//参数

	int connfd;			    //client与server的控制连接fd
	int datafd;			    //client与server的数据连接fd

	int nobodyfd;		//nobody进程所用fd
	int workfd;			//work进程所用fd

}event_t;

extern event_t *pevent;
void EventInit(event_t *ptr);
void EventBegin(event_t *ptr);

#endif
