#include "echo.h"

/*
 * Read - 读取固定字节数
 * fd：文件描述符
 * buf：接受缓存区
 * n：要读取的字节数
 * 成功返回读取的字节数，失败返回-1
 */
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

/*
 * Write - 发送固定字节数
 * fd : 文件描述符
 * buf : 发送缓存区
 * n : 要发送的字节数
 * 成功返回发送的字节数，失败返回-1
 */
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

/*
 * RecvPeek - 窥看套接字缓存区的数据，不移除数据
 * fd：文件描述符
 * buf：接受缓存区
 * n：要读取的字节数
 * 成功返回读取的字节数，失败返回-1
 */
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

/*
 * Readline - 读取一行数据
 * fd：文件描述符
 * buf：接受缓存区
 * maxlen：能读取的最大字节数
 * 成功返回读取的字节数，失败返回-1
 */
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

/*
 *  TcpServer - 返回监听套接字
 *  host：服务器IP地址或服务器主机名
 *  port：服务器端口号
 *  成功返回监听套接字
 */
int TcpServer(const char *host,unsigned short port)
{
	int listenfd;
	if((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		ErrQuit("socket");
	}
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	if(host != NULL){
		if(inet_pton(AF_INET,host,&servaddr.sin_addr) == 0){
			struct hostent *hp = gethostbyname(host);
			if(hp == NULL){
				ErrQuit("gethostbyname");
			}
			servaddr.sin_addr = *(struct in_addr *)(hp->h_addr_list);
		}
	}
	else
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	int on = 1;
	if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) < 0){
			ErrQuit("setsockopt");
	}
	if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0){
			ErrQuit("bind");
	}
	if(listen(listenfd,SOMAXCONN) < 0){
			ErrQuit("listen");
	}
	return listenfd;
}
			
			















			






 
