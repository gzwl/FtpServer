#include "echo.h"


ssize_t Read(int fd,void *buf,size_t n)
{
	size_t nleft = n;
	ssize_t nread;
	while(nleft){
		if((nread = read(fd,buf,nleft)) < 0){
			if(errno == EINTR)	continue;
			else	return -1;
		}
		else if(nread == 0)		break;
		else{
			nleft -= nread;
			buf += nread;
		}
	}
	return n - nleft;
}

ssize_t Write(int fd,const void *buf,size_t n)
{
	size_t nleft = n;
	ssize_t nwrite;
	while(nleft){
		if((nwrite = write(fd,buf,nleft)) <= 0){
			if(errno == EINTR)	continue;
			else	return -1;
		}
		else{
			nleft -= nwrite;
			buf += nwrite;
		}
	}
	return n - nleft;
}

static ssize_t RecvPeek(int fd,void *buf,size_t n)
{
	size_t nleft = n;
	ssize_t nread;
	while(nleft){
		nread = recv(fd,buf,nleft,MSG_PEEK);
		if(nread < 0){
			if(errno == EINTR)	continue;
			else	return -1;
		}
		else if(nread == 0)		break;
		else{
			nleft -= nread;
			buf += nread;
		}
	}
	return n -= nleft;
}

size_t Readline(int fd,void *buf,size_t maxlen)
{
	size_t nleft = maxlen;
	ssize_t nread;
	char *ptr = buf;
	while(nleft){
		nread = RecvPeek(fd,ptr,nleft);
		if(nread < 0)	return -1;
		else if(nread == 0)		break;
		ssize_t i;
		for(i = 0;i < nread;i++){
			if(*(ptr + i) == '\n'){
				nread = Read(fd,ptr,i);
				if(nread != i)	return -1;
				nleft -= nread;
				ptr += nread;
				*ptr = 0;
				return maxlen - nleft;
			}
		}
		nread = Read(fd,ptr,nread);
		if(nread != i)	return -1;
		nleft -= nread;
		ptr += nread;
	}
	*ptr = 0;
	return maxlen - nread;
}
	
			






			






 
