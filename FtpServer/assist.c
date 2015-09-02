#include "assist.h"
#include "common.h"
#include "echo.h"

#include <sys/ioctl.h>
#include <net/if.h>

void ftp_get_local_ip(struct in_addr  *ip)
{
 	int sockfd;
 	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
 		err_quit("socket");
 	}

 	struct ifreq req;

 	memset(&req,0,sizeof(struct ifreq));
 	strcpy(req.ifr_name, "eth0");

 	if(ioctl(sockfd,SIOCGIFADDR,&req) == -1)
 		err_quit("ioctl");

 	struct sockaddr_in *host = (struct sockaddr_in*)&req.ifr_addr;
 	*ip = host->sin_addr;
 	close(sockfd);
 	return ;
}

int ftp_file_read_lock(int fd)
{
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_pid = getpid();
	return fcntl(fd,F_SETLK,&lock);
}

int ftp_file_write_lock(int fd)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_pid = getpid();
	return fcntl(fd,F_SETLK,&lock);
}

int ftp_file_unlock(int fd)
{
	struct flock unlock;
	unlock.l_type = F_UNLCK;
	unlock.l_whence = SEEK_SET;
	unlock.l_start = 0;
	unlock.l_len = 0;
	unlock.l_pid = getpid();
	return fcntl(fd,F_SETLK,&unlock);
}

int NonblockFd(int fd)
{
	int flag;
	if((flag = fcntl(fd,F_GETFL,0)) < 0){
		err_quit("NonblockFd fcntl");
	}
	flag |= O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flag) < 0){
		err_quit("NonblockFd fcntl");
	}
	return 0;
}

int BlockFd(int fd)
{
	int flag;
	if((flag = fcntl(fd,F_GETFL,0)) < 0){
		err_quit("BlockFd fcntl");
	}
	flag &= ~O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flag) < 0){
		err_quit("BlockFd fcntl");
	}
	return 0;
}



