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
