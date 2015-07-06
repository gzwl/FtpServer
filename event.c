#include "event.h"
#include "ftp_nobody.h"
#include "ftp_work.h"
#include "common.h"

event_t *pevent;
void EventInit(event_t *ptr)
{
	memset(ptr->command,0,sizeof(ptr->command));
	memset(ptr->com,0,sizeof(ptr->com));
	memset(ptr->args,0,sizeof(ptr->args));

	ptr->datafd = -1;
	ptr->connfd = -1;
	ptr->nobodyfd = -1;
   	ptr->workfd = -1;

	ptr->login = -1;
	ptr->useruid = -1;
	memset(ptr->username,0,sizeof(ptr->username));
}

void EventBegin(event_t *ptr)
{
	int fd[2];
	if((socketpair(AF_LOCAL,SOCK_STREAM,0,fd)) < 0){
			ErrQuit("socketpair");
	}
	ptr->nobodyfd = fd[0];
	ptr->workfd = fd[1];
	pid_t pid;
	if((pid = fork()) < 0){
		ErrQuit("fork");
	}
	else if(pid){
		NobodyInit(ptr);
		NobodyHandle(ptr);
	}
	else{
		WorkInit(ptr);
		WorkHandle(ptr);	
	}
}

void EventResetCommand(event_t *ptr)
{
	memset(ptr->command,0,sizeof(ptr->command));
	memset(ptr->com,0,sizeof(ptr->com));
	memset(ptr->args,0,sizeof(ptr->args));

}
								
								

