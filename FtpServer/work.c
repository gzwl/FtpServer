#include "work.h"
#include "common.h"
#include "command.h"
#include "echo.h"
#include "event.h"
#include "configure.h"
#include "ftp_string.h"

/**********上半部分是work进程会使用的函数**********/

//超时处理函数
static void HandleAlarm();
static void Alarm(int signo);


//work进程的初始化
void WorkInit(event_t *ptr)
{
	close(ptr->nobodyfd);
	ptr->nobodyfd = -1;	
	HandleAlarm();    		
}


//work进程内部的具体工作
void WorkHandle(event_t *ptr)
{
	FtpReply(ptr,FTP_SERVER_READY,"FtpServer1.0\r\n");	
	while(1){
		EventResetCommand(ptr);
		alarm(Tunable_Recv_Timeout);
		if(Readline(ptr->connfd,ptr->command,1024) < 0)
				ErrQuit("Readline");
		alarm(0);
		//CleanRight(ptr->command);
		sscanf(ptr->command,"%s %s",ptr->com,ptr->args);
		LetterUpper(ptr->com);
		SolveCommand(ptr);	//调用命令处理函数
	}	
}


static void HandleAlarm()
{
	if(signal(SIGALRM,Alarm) == SIG_ERR){
		ErrQuit("signal");
	}
}

static void Alarm(int signo)
{/*
	if(pevent->datafd != -1){
		close(pevent->datafd);
	}
	shutdown(pevent->connfd,SHUT_RD);
	FtpReply(pevent,FTP_CONTROL_CLOSE,"Receive Timeout\r\n");
	shutdown(pevent->connfd,SHUT_WR);
	ErrQuit("Receive Timeout");*/
}

int IpcSendCommand(event_t *ptr,const char cmd)
{
	return Write(ptr->workfd,&cmd,1);
}

int IpcRecvResult(event_t *ptr,char *res)
{
	return Read(ptr->workfd,res,1);
}

int IpcRecvFd(event_t *ptr,int *fd)
{
	return RecvFd(ptr->workfd,fd);
}

int IpcSendDigit(event_t *ptr,int num)
{
	char buf[4];
	sprintf(buf,"%d",num);
	return Write(ptr->workfd,buf,4);
}

/**********下半部分是nobody进程会使用的函数**********/



//接受来自work进程的命令
static int IpcRecvCommand(event_t *ptr,char *cmd);

//向work进程发送命令执行结果
static int IpcSendResult(event_t *ptr,const char res);

//向work进程发送文件描述符
static int IpcSendFd(event_t *ptr,int fd);

//向work进程接收数字
static int IpcRecvDigit(event_t *ptr,int* num); 


//打开监听套接字，PASV模式下使用
static void OpenListenFd(event_t *ptr);

//PASV模式建立数据连接
static void AcceptData(event_t *ptr);

//PORT模式建立数据连接
static void ConnectData(event_t *ptr);

//nobody进程的初始化
void NobodyInit(event_t *ptr)
{
	//关闭多余的文件描述符
	close(ptr->connfd);
	close(ptr->workfd);
	ptr->workfd = -1;

	//设置为nobody进程
	struct passwd *p;
    if((p = getpwnam("nobody")) == NULL){
		ErrQuit("getpwnam");
	}
	if(setegid(p->pw_gid) == -1){
		ErrQuit("NobodyInit setegid");
	}
	if(seteuid(p->pw_uid) == -1){
		ErrQuit("NobodyInit seteuid");
	}

	//为进程添加绑定20端口的权限
	struct __user_cap_header_struct cap_user_header;
	cap_user_header.version = _LINUX_CAPABILITY_VERSION_2;
 	cap_user_header.pid = getpid();
	struct __user_cap_data_struct cap_user_data;
	__u32 cap_mask = 0; 						//类似于权限的集合
	cap_mask |= (1 << CAP_NET_BIND_SERVICE); 	//0001000000
	cap_user_data.effective = cap_mask;
	cap_user_data.permitted = cap_mask;
	cap_user_data.inheritable = 0; 				//子进程不继承特权
	if(capset(&cap_user_header, &cap_user_data) == -1)
			ErrQuit("capset");	
	
}

//nobody进程内部的具体工作
void NobodyHandle(event_t *ptr)
{
	char cmd;
	while(1){
		if(IpcRecvCommand(ptr,&cmd) < 0)	exit(0);
		switch(cmd){
				case IPC_LISTEN_OPEN : OpenListenFd(ptr);break;		//打开监听套接字
				case IPC_ACCEPT		 : AcceptData(ptr);break;		//PASV模式建立数据连接
				case IPC_CONNECT	 : ConnectData(ptr);break;		//PORT模式建立数据连接		
		}
	}


}

static int IpcRecvCommand(event_t *ptr,char *cmd)
{
	if(Read(ptr->nobodyfd,cmd,1) != 1)	return -1;
	return 0;

}

static int IpcSendResult(event_t *ptr,const char res)
{
	return Write(ptr->nobodyfd,&res,1);
}	

static int IpcSendFd(event_t *ptr,int fd)
{
	return SendFd(ptr->nobodyfd,fd);
}

static int IpcRecvDigit(event_t *ptr,int *num)
{
	char buf[4];
	if(Read(ptr->nobodyfd,buf,4) != 4)	return -1;
	sscanf(buf,"%d",num);
	return 0;
}

static void OpenListenFd(event_t *ptr)
{
	struct in_addr ip;
	GetLocalIp(&ip);
	char ipstr[40] = {0};
	inet_ntop(AF_INET,&ip,ipstr,sizeof(ipstr));
	ptr->listenfd = TcpServer(ipstr,20);
	if(ptr->listenfd == -1)
		IpcSendResult(ptr,IPC_COMMAND_BAD);
	else
		IpcSendResult(ptr,IPC_COMMAND_OK);
}

static void AcceptData(event_t *ptr)
{
	int datafd = AcceptTimeout(ptr->listenfd,NULL,Tunable_Data_Timeout);
	close(ptr->listenfd);
	if(datafd <= 0){
		IpcSendResult(ptr,IPC_COMMAND_BAD);
	}
	IpcSendResult(ptr,IPC_COMMAND_OK);
	IpcSendFd(ptr,datafd);
	close(datafd);
}


static void ConnectData(event_t *ptr)
{
	struct sockaddr_in cliaddr;
	memset(&cliaddr,0,sizeof(&cliaddr));
	cliaddr.sin_family = AF_INET;
	IpcRecvDigit(ptr,&cliaddr.sin_addr.s_addr);
	IpcRecvDigit(ptr,&cliaddr.sin_port);
	int datafd = socket(AF_INET,SOCK_STREAM,0);
	if(ConnectTimeout(datafd,&cliaddr,Tunable_Data_Timeout) <= 0){
		IpcSendResult(ptr,IPC_COMMAND_BAD);
	}
	IpcSendResult(ptr,IPC_COMMAND_OK);
	IpcSendFd(ptr,datafd);
	close(datafd);
}

















