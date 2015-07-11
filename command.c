#include "command.h"
#include "event.h"
#include "common.h"
#include "work.h"
#include "configure.h"

typedef struct
{
	const char *text;
	void (*handler)(event_t *ptr);
}ftp_cmd_t;

static void SolveCwd(event_t *ptr);
static void SolveMkd(event_t *ptr);
static void SolveUser(event_t *ptr);
static void SolvePass(event_t *ptr);
static void SolvePort(event_t *ptr);
static void SolvePasv(event_t *ptr);
static void SolveQuit(event_t *ptr);
static void SolveRmd(event_t *ptr);


static ftp_cmd_t ftp_cmd_s[] = {
	
	{"CWD"	,	SolveCwd},	
	
	{"MKD"	,	SolveMkd},

	{"PASS"	,	SolvePass},
	{"PASV"	,	SolvePasv},
	{"PORT" ,   SolvePort},

	{"QUIT" ,   SolveQuit},

	{"RMD"	,	SolveRmd},

	{"USER" ,   SolveUser}

};


void FtpReply(event_t *ptr,int status,const char* text)
{
	char buf[1024] = {0};
	snprintf(buf,sizeof(buf),"%d %s\r\n",status,text);
	Write(ptr->connfd,buf,strlen(buf));
}


void SolveCommand(event_t *ptr)
{
	 //未登录 - 未输入帐号	
	 if(ptr->login == -1 && strcmp(ptr->com,"USER") != 0){
			FtpReply(ptr,FTP_LOGIN_ERR,"Please input your username\r\n");
			return ;
	 }

	 //未登录 - 未输入密码
 	 if(ptr->login == 0  && strcmp(ptr->com,"PASS") != 0){
	 		FtpReply(ptr,FTP_LOGIN_ERR,"Not logged in\r\n");
			ptr->login = -1;
			return ;
	 }

	 //查找命令并执行	 
	 size_t n = sizeof(ftp_cmd_s)/sizeof(ftp_cmd_t);
	 size_t i;
	 for(i = 0;i < n;i++){
	 	if(strcmp(ptr->com,ftp_cmd_s[i].text) == 0){
				ftp_cmd_s[i].handler(ptr);
				return ;
		}
	 }
	 FtpReply(ptr,FTP_COMMAND_ERR,"Command no found\r\n");
}

static void SolveCwd(event_t *ptr)
{
	if(chdir(ptr->args) < 0){
		FtpReply(ptr,FTP_COMMAND_FAIL,"Change directory fail\r\n");
	}
	else{
		FtpReply(ptr,FTP_COMMAND_SUCCESS,"Change directory success\r\n");
	}

}

static void SolveMkd(event_t *ptr)
{
	if(rmdir(ptr->args) < 0){
		FtpReply(ptr,FTP_COMMAND_FAIL,"Remove directory fail\r\n");
	}
	else{
		FtpReply(ptr,FTP_COMMAND_SUCCESS,"Remove directory success\r\n");
	}

}

static void SolveUser(event_t *ptr)
{
	//已经登入返回错误
	if(ptr->login == 1){
		FtpReply(ptr,FTP_COMMAND_SEQ,"Logged in already\r\n");
		return ;
	}

	struct passwd *p = getpwnam(ptr->args);
	if(p == NULL){
		FtpReply(ptr,FTP_LOGIN_ERR,"Username incorrect\r\n");
	}
	else{
		FtpReply(ptr,FTP_USERNAME_OK,"Please input password\r\n");
		ptr->login = 0;
		ptr->useruid = p->pw_uid;
	}
}

static void SolvePass(event_t *ptr)
{
	//已经登入返回错误
	if(ptr->login == 1){
		FtpReply(ptr,FTP_COMMAND_SEQ,"Logged in already\r\n");
		return ;
	}
	
	struct passwd *p = getpwuid(ptr->useruid);
	if(strcmp(p->pw_passwd,ptr->args)){
		FtpReply(ptr,FTP_LOGIN_ERR,"Password incorrect\r\n");
		ptr->login = -1;
		ptr->useruid = -1;
	}
	else{
		ptr->login = 1;
		strcpy(ptr->username,p->pw_name);
		if(setegid(p->pw_gid) == -1){
			ErrQuit("setegid");
		}
		if(seteuid(p->pw_uid) == -1){
			ErrQuit("seteuid");
		}
		if(chdir(p->pw_dir) < 0){
			ErrQuit("chdir");
		}
		umask(Tunable_Local_Umask);
		FtpReply(ptr,FTP_LOGIN_SUCCESS,"Login successful\r\n");
		
	}
}

static void SolvePort(event_t *ptr)
{
	ptr->addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	memset(ptr->addr,0,sizeof(ptr->addr));
	unsigned int  tmp[6];	
	sscanf(ptr->args,"%d,%d,%d,%d,%d,%d,",&tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
	ptr->addr->sin_family = AF_INET;
	unsigned char *p = (unsigned char *)ptr->addr->sin_port;
	p[0] = tmp[4];
	p[1] = tmp[5];
	p = (unsigned char *)&ptr->addr->sin_addr;
	p[0] = tmp[0];
	p[1] = tmp[1];
	p[2] = tmp[2];
	p[3] = tmp[3];
	FtpReply(ptr,FTP_PORT_OK,"Enter port mode success\r\n");	
}

static void SolvePasv(event_t *ptr)
{
	struct in_addr ip;
	GetLocalIp(&ip);
	IpcSendCommand(IPC_LISTEN_OPEN);
	char res;
	IpcRecvResult(&res);
	if(res == IPC_COMMAND_BAD)
		FtpReply(ptr,FTP_COMMAND_FAIL,"Enter pasv mode fail\r\n");
	char addr[20];
	inet_ntop(AF_INET,&ip,addr,sizeof(addr));
	uint32_t tmp[6];
	sscanf(addr,"%d.%d.%d.%d",&tmp[0],&tmp[1],&tmp[2],&tmp[3]);
    uint16_t port = htons(20);
    unsigned char *p = (unsigned char *)&port;
	tmp[4] = p[0];
	tmp[5] = p[1];
    char text[1024] = {0};
	sprintf(text,"Enter pasv mode success (%d,%d,%d,%d,%d,%d)\r\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
	FtpReply(ptr,FTP_PASV_OK,text);
	
}


static void SolveQuit(event_t *ptr)
{
	FtpReply(ptr,FTP_OVER,"Good bye");
	exit(0);
}	

static void SolveRmd(event_t *ptr)
{
	if(rmdir(ptr->args) < 0){
		FtpReply(ptr,FTP_COMMAND_FAIL,"Remove directory fail");
	}
	else{
		FtpReply(ptr,FTP_COMMAND_SUCCESS,"Remove directory success");
	}
}





















