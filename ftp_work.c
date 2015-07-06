#include "ftp_work.h"
#include "common.h"
#include "command.h"
#include "echo.h"
#include "event.h"
#include "configure.h"
#include "ftp_string.h"

static void HandleAlarm();
static void Alarm(int signo);

void WorkInit(event_t *ptr)
{
	close(ptr->nobodyfd);
	ptr->nobodyfd = -1;	
	HandleAlarm();    		
}

void WorkHandle(event_t *ptr)
{
	FtpReply(ptr,FTP_SERVER_READY,"FtpServer1.0\r\n");	
	while(1){
		EventResetCommand(ptr);
		alarm(Tunable_Recv_Timeout);
		if(Readline(ptr->connfd,ptr->command,1024) < 0)
				ErrQuit("Readline");
		alarm(0);
		CleanRight(ptr->command);
		sscanf(ptr->command,"%s %s",ptr->com,ptr->args);
		SolveCommand(ptr);
	}	
}


static void HandleAlarm()
{
	if(signal(SIGALRM,Alarm) == SIG_ERR){
		ErrQuit("signal");
	}
}

static void Alarm(int signo)
{
	if(pevent->datafd != -1){
		close(pevent->datafd);
	}
	shutdown(pevent->connfd,SHUT_RD);
	FtpReply(pevent,FTP_CONTROL_CLOSE,"Receive Timeout\r\n");
	shutdown(pevent->connfd,SHUT_WR);
	ErrQuit("Receive Timeout");
}

