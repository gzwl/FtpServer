#include "assist.h"
#include "common.h"
#include "echo.h"

void CheckRoot()
{
	if(getuid()){
		fprintf(stderr,"FtpServer must be started by ROOT!\n");
		exit(EXIT_FAILURE);
	}
}

static void Sigchld(int signo)
{
	pid_t pid;
	while(waitpid(-1,0,WNOHANG) > 0);
}

void HandleSigchld()
{
	if(signal(SIGCHLD,Sigchld) == SIG_ERR){
		ErrQuit("signal");
	}
}

void GetLocalIp(struct in_addr  *ip)
{
	char name [32];
	gethostname(name,sizeof(name));
	struct hostent *p = gethostbyname(name);
	if(p == NULL){
		ErrQuit("GetLocalIP gethostbyname");
	}
	memcpy(ip,p->h_addr_list[0],sizeof(ip));
}
