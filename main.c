#include "echo.h"
#include "assist.h"
#include "configure.h"
#include "event.h"
#include "common.h"


int main(int argc,char **argv)
{
	CheckRoot();
	HandleSigchld();
	int listenfd = TcpServer(Tunable_Listen_Address,Tunable_Listen_Port);
	while(1){
		struct sockaddr_in cliaddr;
		socklen_t len;
		int connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&len);
		if(connfd < 0){
			if(errno == EINTR)	continue;
			else	ErrQuit("accept");
		}

		pid_t pid;
		if((pid = fork()) < 0){
			ErrQuit("fork");
		}
		else if(pid == 0){
			close(listenfd);
			event_t eve;
			EventInit(&eve);
			pevent = &eve;
			pevent->connfd = connfd;
			EventBegin(pevent);
		}
		else{
			close(connfd);

		}
	}
}

