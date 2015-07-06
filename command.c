#include "command.h"
#include "event.h"
#include "common.h"

typedef struct
{
	const char *text;
	void (*handler)(event_t *ptr);
}ftp_cmd_t;

static void SolveUser(event_t *ptr);
static void SolvePass(event_t *ptr);
static void SolvePort(event_t *ptr);
static void SolvePasv(event_t *ptr);
static void SolveQuit(event_t *ptr);
static ftp_cmd_t ftp_cmd_s[] = {
		
	{"USER"	,	SolveUser},
	{"PASS"	,	SolvePass},

	{"PORT"	,	SolvePort},
	{"PASV"	,	SolvePasv},

	{"Quit" ,   SolveQuit}

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
			FtpReply(ptr,FTP_LOGIN_ERR,"Not logged in\r\n");
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
	if(p == NULL){
		FtpReply(ptr,FTP_LOGIN_ERR,"Password incorrect\r\n");
		ptr->login = -1;
		ptr->useruid = -1;
	}
	else{
		FtpReply(ptr,FTP_LOGIN_SUCCESS,"Login successful\r\n");
		ptr->login = 1;
		strcpy(ptr->username,p->pw_name);
		if(setegid(p->pw_gid) == -1){
			ErrQuit("setegid");
		}
		if(seteuid(p->pw_uid) == -1){
			ErrQuit("seteuid");
		}
	}
}

static void SolvePort(event_t *ptr)
{


}

static void SolvePasv(event_t *ptr)
{


}


static void SolveQuit(event_t *ptr)
{
	printf("Your mother fuck\n");

}























