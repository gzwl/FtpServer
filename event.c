#include "event.h"
#include "ftp_nobody.h"
#include "ftp_work.h"
#include "echo.h"

event_t *pevent;
void EventInit(event_t *ptr)
{
	ptr->datafd = -1;
	ptr->connfd = -1;
	ptr->nobodyfd = -1;
   	ptr->workfd = -1;
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
								
								

