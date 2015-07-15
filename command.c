#include "command.h"
#include "event.h"
#include "common.h"
#include "work.h"
#include "configure.h"
#include "transfer.h"

typedef struct
{
	const char *text;
	void (*handler)(event_t *ptr);
}ftp_cmd_t;

static void SolveCwd(event_t *ptr);
static void SolveFeat(event_t *ptr);
static void SolveList(event_t *ptr);
static void SolveMkd(event_t *ptr);
static void SolveUser(event_t *ptr);
static void SolvePass(event_t *ptr);
static void SolvePort(event_t *ptr);
static void SolvePasv(event_t *ptr);
static void SolvePwd(event_t *ptr);
static void SolveNlst(event_t *ptr);
static void SolveNoop(event_t *ptr);
static void SolveQuit(event_t *ptr);
static void SolveRmd(event_t *ptr);
static void SolveRetr(event_t *ptr);
static void SolveSyst(event_t *ptr);
static void SolveType(event_t *ptr);

static ftp_cmd_t ftp_cmd_s[] = {
	
	{"CWD"	,	SolveCwd},	
	
	{"LIST" ,	SolveList},

	{"MKD"	,	SolveMkd},

	{"NLST" ,	SolveNlst},
	{"NOOP" ,   SolveNoop},

	{"PASS"	,	SolvePass},
	{"PASV"	,	SolvePasv},
	{"PORT" ,   SolvePort},
	{"PWD"  ,	SolvePwd},
	
	{"QUIT" ,   SolveQuit},

	{"RETR" ,   SolveRetr},
	{"RMD"	,	SolveRmd},

	{"SYST" ,	SolveSyst},

	{"TYPE" ,   SolveType},

	{"USER" ,   SolveUser}

};


void FtpReply(event_t *ptr,int status,const char* text)
{
	char buf[1024] = {0};
	snprintf(buf,sizeof(buf),"%d %s\r\n",status,text);
	Write(ptr->connfd,buf,strlen(buf));
}

void FtpLreply(event_t *ptr,int status,const char *text)
{
	char buf[1024] = {0};
	snprintf(buf,sizeof(buf),"%d-%s\r\n",status,text);
	Write(ptr->connfd,buf,strlen(buf));
}


void SolveCommand(event_t *ptr)
{
	 //未登录 - 未输入帐号	
	 if(ptr->login == -1 && strcmp(ptr->com,"USER") != 0){
			FtpReply(ptr,FTP_LOGIN_ERR,"Please input your username");
			return ;
	 }

	 //未登录 - 未输入密码
 	 if(ptr->login == 0  && strcmp(ptr->com,"PASS") != 0){
	 		FtpReply(ptr,FTP_LOGIN_ERR,"Not logged in");
			ptr->login = -1;
			return ;
	 }

	 //二分查找命令并执行	 
	 size_t n = sizeof(ftp_cmd_s)/sizeof(ftp_cmd_t);
	 int lhs = -1,rhs  = n;
	 while(rhs - lhs > 1){

		int mid = (lhs + rhs)/2;
		int tmp = strcmp(ptr->com,ftp_cmd_s[mid].text);
	 	if(tmp == 0){
				ftp_cmd_s[mid].handler(ptr);
				return ;
		}
		else if(tmp > 0)	lhs = mid;
		else				rhs = mid;
	 }
	 FtpReply(ptr,FTP_COMMAND_ERR,"Command no found");
}

static void SolveCwd(event_t *ptr)
{
	if(chdir(ptr->args) < 0){
		FtpReply(ptr,FTP_FILE_FAIL,"Change directory fail");
	}
	else{
		FtpReply(ptr,FTP_FILE_SUCCESS,"Change directory success");
	}

}


static void SolveList(event_t *ptr)
{
	SendList(ptr,0);
}

static void SolveMkd(event_t *ptr)
{
	if(rmdir(ptr->args) < 0){
		FtpReply(ptr,FTP_COMMAND_FAIL,"Remove directory fail");
	}
	else{
		FtpReply(ptr,FTP_COMMAND_SUCCESS,"Remove directory success");
	}

}

static void SolveNlst(event_t *ptr)
{
	SendList(ptr,1);
}

static void SolveNoop(event_t *ptr)
{
	FtpReply(ptr,FTP_SERVER_READY,"FtpServer1.0");
}


static void SolveUser(event_t *ptr)
{
	//已经登入返回错误
	if(ptr->login == 1){
		FtpReply(ptr,FTP_COMMAND_SEQ,"Logged in already");
		return ;
	}

	struct passwd *p = getpwnam(ptr->args);
	if(p == NULL){
		FtpReply(ptr,FTP_LOGIN_ERR,"Username incorrect");
	}
	else{
		FtpReply(ptr,FTP_USERNAME_OK,"Please input password");
		ptr->login = 0;
		ptr->useruid = p->pw_uid;
	}
}

static void SolvePass(event_t *ptr)
{
	//已经登入返回错误
	if(ptr->login == 1){
		FtpReply(ptr,FTP_COMMAND_SEQ,"Logged in already");
		return ;
	}
	
	struct passwd *p = getpwuid(ptr->useruid);
	struct spwd *sp = getspnam(p->pw_name);
	if(sp == NULL){
		FtpReply(ptr,FTP_LOGIN_ERR,"Password incorrect");
		return;
	}
		
	char *encryption = crypt(ptr->args,sp->sp_pwdp);
	if(strcmp(encryption,sp->sp_pwdp)){
		FtpReply(ptr,FTP_LOGIN_ERR,"Password incorrect");
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
		FtpReply(ptr,FTP_LOGIN_SUCCESS,"Login successful");
		
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
	ptr->port = 1;
	FtpReply(ptr,FTP_PORT_OK,"Enter port mode success");	
}

static void SolvePasv(event_t *ptr)
{
	struct in_addr ip;
	GetLocalIp(&ip);
	IpcSendCommand(ptr,IPC_LISTEN_OPEN);
	char res;
	IpcRecvResult(ptr,&res);
	if(res == IPC_COMMAND_BAD)
		FtpReply(ptr,FTP_COMMAND_FAIL,"Enter pasv mode fail");
	char addr[20];
	inet_ntop(AF_INET,&ip,addr,sizeof(addr));
	uint32_t tmp[6];
	sscanf(addr,"%d.%d.%d.%d",&tmp[0],&tmp[1],&tmp[2],&tmp[3]);
    uint16_t port = htons(20);
    unsigned char *p = (unsigned char *)&port;
	tmp[4] = p[0];
	tmp[5] = p[1];
    char text[1024] = {0};
	sprintf(text,"Enter pasv mode success (%d,%d,%d,%d,%d,%d)",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
	FtpReply(ptr,FTP_PASV_OK,text);
	ptr->pasv = 1;
	
}

static void SolvePwd(event_t *ptr)
{
	char buf[128] = {0};
	if(getcwd(buf,128) == NULL){
		FtpReply(ptr,FTP_COMMAND_FAIL,"Print work directory fail");
	}
	else{
		char text[128];
		sprintf(text,"\"%s\"",buf);
		FtpReply(ptr,FTP_PWD_OK,text);
	}
}

static void SolveQuit(event_t *ptr)
{
	FtpReply(ptr,FTP_OVER,"Good bye");
	exit(0);
}	

static void SolveRetr(event_t *ptr)
{
	DownloadFile(ptr);
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

static void SolveSyst(event_t *ptr)
{
	FtpReply(ptr,FTP_COMMAND_SUCCESS,"UNIX Type : L8");

}

static void SolveType(event_t *ptr)
{
	if(strcmp(ptr->args,"A") == 0){
		ptr->transmode = 0;
		FtpReply(ptr,FTP_COMMAND_SUCCESS,"Switch to ASCII mode");
	}
	else if(strcmp(ptr->args,"I") == 0){
		ptr->transmode = 1;
		FtpReply(ptr,FTP_COMMAND_SUCCESS,"Switch to Binary mode");
	}
	else{
		FtpReply(ptr,FTP_PARAMETER_BAD,"Unrecognized parameter");
	}

}





















