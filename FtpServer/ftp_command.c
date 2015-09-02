#include "ftp_command.h"
#include "ftp_event.h"
#include "ftp_process.h"
#include "ftp_channel.h"
#include "common.h"
#include "configure.h"
#include "ftp_transfer.h"
#include "ftp_epoll.h"

static void ftp_solve_command(ftp_event_t* ptr);
static void solve_appe(ftp_event_t *ptr);
static void solve_cwd(ftp_event_t *ptr);
static void solve_dele(ftp_event_t *ptr);
static void solve_feat(ftp_event_t *ptr);
static void solve_list(ftp_event_t *ptr);
static void solve_mkd(ftp_event_t *ptr);
static void solve_user(ftp_event_t *ptr);
static void solve_pass(ftp_event_t *ptr);
static void solve_port(ftp_event_t *ptr);
static void solve_pasv(ftp_event_t *ptr);
static void solve_pwd(ftp_event_t *ptr);
static void solve_nlst(ftp_event_t *ptr);
static void solve_noop(ftp_event_t *ptr);
static void solve_quit(ftp_event_t *ptr);
static void solve_rest(ftp_event_t *ptr);
static void solve_rmd(ftp_event_t *ptr);
static void solve_retr(ftp_event_t *ptr);
static void solve_size(ftp_event_t *ptr);
static void solve_stor(ftp_event_t *ptr);
static void solve_syst(ftp_event_t *ptr);
static void solve_type(ftp_event_t *ptr);

static void ftp_reset_command();
static int ftp_solve_pasv_result(ftp_event_t* ptr);

typedef struct
{
	const char *text;
	void (*handler)(ftp_event_t *ptr);
}ftp_cmd_t;

static ftp_cmd_t ftp_cmd_s[] = {

	{"APPE" ,	solve_appe},

	{"CWD"	,	solve_cwd},

	{"Dele" ,	solve_dele},

	{"LIST" ,	solve_list},

	{"MKD"	,	solve_mkd},

	{"NLST" ,	solve_nlst},
	{"NOOP" ,   solve_noop},

	{"PASS"	,	solve_pass},
	{"PASV"	,	solve_pasv},
	{"PORT" ,   solve_port},
	{"PWD"  ,	solve_pwd},

	{"QUIT" ,   solve_quit},

	{"REST"	,	solve_rest},
	{"RETR" ,   solve_retr},
	{"RMD"	,	solve_rmd},

	{"SIZE" ,	solve_size},
	{"STOR"	,	solve_stor},
	{"SYST" ,	solve_syst},

	{"TYPE" ,   solve_type},

	{"USER" ,   solve_user}

};

//work进程内部的具体工作
int ftp_request_handler(ftp_event_t *ptr)
{
    ftp_reset_command();
    if(readline(ptr->fd,ftp_connection.command,MAX_LEN) <= 0){

    }
    sscanf(ftp_connection.command,"%s %s",ftp_connection.com,ftp_connection.args);
    ftp_letter_upper(ftp_connection.com);
    ftp_solve_command(ptr);	//调用命令处理函数
}

void ftp_reset_command()
{
	memset(ftp_connection.command,0,sizeof(ftp_connection.command));
	memset(ftp_connection.com,0,sizeof(ftp_connection.com));
	memset(ftp_connection.args,0,sizeof(ftp_connection.args));
}

void ftp_reply(int status,const char* text)
{
	char buf[1024] = {0};
	snprintf(buf,sizeof(buf),"%d %s\r\n",status,text);
	writen(ftp_connection.connfd,buf,strlen(buf));
}

void ftp_lreply(int status,const char *text)
{
	char buf[1024] = {0};
	snprintf(buf,sizeof(buf),"%d-%s\r\n",status,text);
	writen(ftp_connection.connfd,buf,strlen(buf));
}


void ftp_solve_command(ftp_event_t *ptr)
{
	 //未登录 - 未输入帐号
	 if(ftp_connection.login == -1 && strcmp(ftp_connection.com,"USER") != 0){
			ftp_reply(FTP_LOGIN_ERR,"Please input your username");
			return ;
	 }

	 //未登录 - 未输入密码
 	 if(ftp_connection.login == 0  && strcmp(ftp_connection.com,"PASS") != 0){
	 		ftp_reply(FTP_LOGIN_ERR,"Not logged in");
			ftp_connection.login = -1;
			return ;
	 }

	 //二分查找命令并执行
	 size_t n = sizeof(ftp_cmd_s)/sizeof(ftp_cmd_t);
	 int lhs = -1,rhs  = n;
	 while(rhs - lhs > 1){

		int mid = (lhs + rhs)/2;
		int tmp = strcmp(ftp_connection.com,ftp_cmd_s[mid].text);
	 	if(tmp == 0){
				ftp_cmd_s[mid].handler(ptr);
				return ;
		}
		else if(tmp > 0)	lhs = mid;
		else				rhs = mid;
	 }
	 ftp_reply(FTP_COMMAND_ERR,"Command no found");
}

static void solve_appe(ftp_event_t *ptr)
{
	ftp_get_data_fd(ptr,FTP_CMD_APPE);
}

static void solve_cwd(ftp_event_t *ptr)
{
	if(chdir(ftp_connection.args) < 0){
		ftp_reply(FTP_FILE_FAIL,"Change directory fail");
	}
	else{
		ftp_reply(FTP_FILE_SUCCESS,"Change directory success");
	}

}

static void solve_dele(ftp_event_t *ptr)
{
	if(unlink(ftp_connection.args) < 0){
		ftp_reply(FTP_FILE_ERR,"Delete file fail");
	}
	else{
		ftp_reply(FTP_FILE_SUCCESS,"Delete file success");
	}

}

static void solve_list(ftp_event_t *ptr)
{
	ftp_get_data_fd(ptr,FTP_CMD_LIST);
}

static void solve_mkd(ftp_event_t *ptr)
{
	if(rmdir(ftp_connection.args) < 0){
		ftp_reply(FTP_COMMAND_FAIL,"Remove directory fail");
	}
	else{
		ftp_reply(FTP_COMMAND_SUCCESS,"Remove directory success");
	}

}

static void solve_nlst(ftp_event_t *ptr)
{
	ftp_get_data_fd(ptr,FTP_CMD_NLST);
}

static void solve_noop(ftp_event_t *ptr)
{
	ftp_reply(FTP_SERVER_READY,"FtpServer1.0");
}


static void solve_user(ftp_event_t *ptr)
{
	//已经登入返回错误
	if(ftp_connection.login == 1){
		ftp_reply(FTP_COMMAND_SEQ,"Logged in already");
		return ;
	}

	struct passwd *p = getpwnam(ftp_connection.args);
	if(p == NULL){
		ftp_reply(FTP_LOGIN_ERR,"Username incorrect");
	}
	else{
		ftp_reply(FTP_USERNAME_OK,"Please input password");
		ftp_connection.login = 0;
		ftp_connection.useruid = p->pw_uid;
	}
}

static void solve_pass(ftp_event_t *ptr)
{
	//已经登入返回错误
	if(ftp_connection.login == 1){
		ftp_reply(FTP_COMMAND_SEQ,"Logged in already");
		return ;
	}

	struct passwd *p = getpwuid(ftp_connection.useruid);
	struct spwd *sp = getspnam(p->pw_name);
	if(sp == NULL){
		ftp_reply(FTP_LOGIN_ERR,"Password incorrect");
		return;
	}

	char *encryption = crypt(ftp_connection.args,sp->sp_pwdp);
	if(strcmp(encryption,sp->sp_pwdp)){
		ftp_reply(FTP_LOGIN_ERR,"Password incorrect");
		ftp_connection.login = -1;
		ftp_connection.useruid = -1;
	}
	else{
		ftp_connection.login = 1;
		strcpy(ftp_connection.username,p->pw_name);
		if(setegid(p->pw_gid) == -1){
			err_quit("setegid");
		}
		if(seteuid(p->pw_uid) == -1){
			err_quit("seteuid");
		}
		if(chdir(p->pw_dir) < 0){
			err_quit("chdir");
		}
		umask(local_umask);
		ftp_reply(FTP_LOGIN_SUCCESS,"Login successful");

	}
}

static void solve_port(ftp_event_t *ptr)
{
	ftp_connection.addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	memset(ftp_connection.addr,0,sizeof(ftp_connection.addr));
	unsigned int  tmp[6];
	sscanf(ftp_connection.args,"%d,%d,%d,%d,%d,%d,",&tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
	ftp_connection.addr->sin_family = AF_INET;
	unsigned char *p = (unsigned char *)ftp_connection.addr->sin_port;
	p[0] = tmp[4];
	p[1] = tmp[5];
	p = (unsigned char *)&ftp_connection.addr->sin_addr;
	p[0] = tmp[0];
	p[1] = tmp[1];
	p[2] = tmp[2];
	p[3] = tmp[3];
	ftp_connection.port = 1;
	ftp_reply(FTP_PORT_OK,"Enter port mode success");
}

static void solve_pasv(ftp_event_t *ptr)
{
	if(ftp_ipc_send_msg(ftp_connection.nobodyfd,IPC_LISTEN_OPEN,NULL) == FTP_ERROR) {
        err_quit("ftp_ipc_send_msg");
	}

    ftp_connection.nobody->read = 1;
    ftp_connection.nobody->read_handler = ftp_solve_pasv_result;
    ftp_epoll_add_event(ftp_connection.nobody,FTP_READ_EVENT);
}

static int ftp_solve_pasv_result(ftp_event_t* ptr)
{
	struct in_addr ip;
	ftp_get_local_ip(&ip);

	int res;
	while(ftp_ipc_recv_msg(ftp_connection.nobodyfd,&res,NULL) == FTP_ERROR) {
        if(errno == EINTR)  continue;
        else return FTP_ERROR;
	}

	if(res == FTP_IPC_BAD)
		ftp_reply(FTP_COMMAND_FAIL,"Enter pasv mode fail");

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
	ftp_reply(FTP_PASV_OK,text);
	ftp_connection.pasv = 1;

	ftp_epoll_del_event(ptr,FTP_READ_EVENT);
	return FTP_OK;
}

static void solve_pwd(ftp_event_t *ptr)
{
	char buf[128] = {0};
	if(getcwd(buf,128) == NULL){
		ftp_reply(FTP_COMMAND_FAIL,"Print work directory fail");
	}
	else{
		char text[128];
		snprintf(text,sizeof(text),"\"%s\"",buf);
		ftp_reply(FTP_PWD_OK,text);
	}
}

static void solve_quit(ftp_event_t *ptr)
{
	ftp_reply(FTP_OVER,"Good bye");
	exit(0);
}

static void solve_rest(ftp_event_t *ptr)
{
	ftp_connection.restart_pos = atoll(ftp_connection.args);
	ftp_reply(FTP_REST_OK,"Set restart position ok");
}

static void solve_retr(ftp_event_t *ptr)
{
	ftp_get_data_fd(ptr,FTP_CMD_RETR);
}

static void solve_rmd(ftp_event_t *ptr)
{
	if(rmdir(ftp_connection.args) < 0){
		ftp_reply(FTP_COMMAND_FAIL,"Remove directory fail");
	}
	else{
		ftp_reply(FTP_COMMAND_SUCCESS,"Remove directory success");
	}
}

static void solve_size(ftp_event_t *ptr)
{
	struct stat buf;
	if(stat(ftp_connection.args,&buf) < 0){
		ftp_reply(FTP_FILE_ERR,"File name error");
	}
	else if(!S_ISREG(buf.st_mode)){
		ftp_reply(FTP_FILE_ERR,"Not regular file");
	}
	else{
		char text[64];
		snprintf(text,sizeof(text),"The size is: %u",buf.st_size);
		ftp_reply(FTP_SIZE_OK,text);
	}
}

static void solve_stor(ftp_event_t *ptr)
{
	ftp_get_data_fd(ptr,FTP_CMD_STOR);
}

static void solve_syst(ftp_event_t *ptr)
{
	ftp_reply(FTP_COMMAND_SUCCESS,"UNIX Type : L8");
}

static void solve_type(ftp_event_t *ptr)
{
	if(strcmp(ftp_connection.args,"A") == 0){
		ftp_connection.transmode = 0;
		ftp_reply(FTP_COMMAND_SUCCESS,"Switch to ASCII mode");
	}
	else if(strcmp(ftp_connection.args,"I") == 0){
		ftp_connection.transmode = 1;
		ftp_reply(FTP_COMMAND_SUCCESS,"Switch to Binary mode");
	}
	else{
		ftp_reply(FTP_PARAMETER_BAD,"Unrecognized parameter");
	}

}

