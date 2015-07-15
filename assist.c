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
	char name [32] = {0};
	gethostname(name,sizeof(name));
	struct hostent *p = gethostbyname(name);
	if(p == NULL){
		ErrQuit("GetLocalIP gethostbyname");
	}
	memcpy(ip,p->h_addr_list[0],sizeof(struct in_addr));
}

int FileReadLock(int fd)
{
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	return fcntl(fd,F_SETLK,&lock);
}

int FileWriteLock(int fd)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	return fcntl(fd,F_SETLK,&lock);
}

int FileUnlock(int fd)
{
	struct flock unlock;
	unlock.l_type = F_UNLCK;
	unlock.l_whence = SEEK_SET;
	unlock.l_start = 0;
	unlock.l_len = 0;
	return fcntl(fd,F_SETLK,&unlock);
}

int NonblockFd(int fd)
{
	int flag;
	if((flag = fcntl(fd,F_GETFL,0)) < 0){
		ErrQuit("NonblockFd fcntl");
	}
	flag |= O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flag) < 0){
		ErrQuit("NonblockFd fcntl");
	}
	return 0;
}

int BlockFd(int fd)
{
	int flag;
	if((flag = fcntl(fd,F_GETFL,0)) < 0){
		ErrQuit("BlockFd fcntl");
	}
	flag &= ~O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flag) < 0){
		ErrQuit("BlockFd fcntl");
	}
	return 0;
}

























