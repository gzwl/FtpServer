#include "echo.h"
#include "common.h"
#include "assist.h"

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
ssize_t writen(int fd,const void *buf,size_t n)
{
	size_t nleft = n;
	ssize_t nwrite;
	while(nleft){
		if((nwrite = write(fd,buf,nleft)) < 0){
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
	ssize_t nread;
	while(1){
		nread = recv(fd,buf,n,MSG_PEEK);
		if(nread < 0 && errno == EINTR)		continue;
		break;
	}
	return nread;
}

/*
 * Readline - 读取一行数据
 * fd：文件描述符
 * buf：接受缓存区
 * maxlen：能读取的最大字节数
 * 成功返回读取的字节数，失败返回-1
 */
ssize_t readline(int fd,void *buf,size_t maxlen)
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
				nread = Read(fd,ptr,i + 1);
				if(nread != i + 1)		return -1;
				nleft -= nread;
				ptr += nread;
				*ptr = '\0';
				return maxlen - nleft;
			}
		}
		nread = Read(fd,ptr,nread);
		if(nread != i)	return -1;
		nleft -= nread;
		ptr += nread;
	}
	*ptr = '\0';
	return maxlen - nleft;

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
		return -1;
	}
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	if(host != NULL){
		if(inet_pton(AF_INET,host,&servaddr.sin_addr) == 0){
			struct hostent *hp = gethostbyname(host);
			if(hp == NULL){
				return -1;
			}
			servaddr.sin_addr = *(struct in_addr *)(hp->h_addr_list);
		}
	}
	else
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	int on = 1;
	if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) < 0){
			return -1;
	}
	if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0){
			return -1;
	}
	if(listen(listenfd,SOMAXCONN) < 0){
			return -1;
	}
	return listenfd;
}


/*
 * SendFd - 向目标进程发送文件描述符
 * des：发送套接字
 * fd ：需要发送的文件描述符
 * 成功返回发送的字节数，失败返回-1
 */

int SendFd(int des,int fd)
{
	struct msghdr msg;
	struct cmsghdr *pmsg;
	struct iovec vec;

	char ms[CMSG_SPACE(sizeof(fd))];
	char sendchar;

	vec.iov_base = &sendchar;
	vec.iov_len = sizeof(sendchar);

	msg.msg_control = ms;
	msg.msg_controllen = sizeof(ms);

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	pmsg = CMSG_FIRSTHDR(&msg);
	pmsg->cmsg_level = SOL_SOCKET;
	pmsg->cmsg_type = SCM_RIGHTS;
	pmsg->cmsg_len = CMSG_LEN(sizeof(fd));

	int *p = (int*)CMSG_DATA(pmsg);
	*p = fd;
	return sendmsg(des,&msg,0);
}


/*
 *  RecvFd - 接受来自目标进程的文件描述符
 *  des : 接受套接字
 *  fd  : 需要接受的文件描述符地址
 *  成功返回0,失败返回-1
 */
int RecvFd(const int des,int* fd)
{
	struct msghdr msg;
	struct cmsghdr *pmsg;
	struct iovec vec;

	char recvchar;
	char ms[CMSG_SPACE(sizeof(int))];

	vec.iov_base = &recvchar;
	vec.iov_len = sizeof(recvchar);

	msg.msg_control = ms;
	msg.msg_controllen = sizeof(ms);

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
    if(recvmsg(des,&msg,0) < 0)		return -1;
	pmsg = CMSG_FIRSTHDR(&msg);
	*fd = *(int*)CMSG_DATA(pmsg);
	return 0;
}

/*
 * 	AcceptTimeout - 返回下一个已完成连接(带超时)
 *  sockfd  : 监听套接字
 *  cliaddr : 客户端地址和协议
 *  timeout : 等待时间
 *  成功返回套接字，超时返回0，出错返回-1
 */
int AcceptTimeout(int sockfd,struct sockaddr_in *cliaddr,int timeout)
{
	if(timeout > 0){
		struct timeval t;
		t.tv_sec = timeout;
		t.tv_usec = 0;
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(sockfd,&rset);
		int res;
		do{
			res = select(sockfd + 1,&rset,NULL,NULL,&t);
		}while(res < 0 && errno == EINTR);
		if(res <= 0)	return res;
		int len = sizeof(struct sockaddr_in);
		return accept(sockfd,(struct sockaddr*)cliaddr,&len);
	}
	else{
		int len = sizeof(struct sockaddr_in);
		return accept(sockfd,(struct sockaddr*)cliaddr,&len);
	}
}


/*
 *	ConnectTimeout - 像目标发起连接
 *	sockfd  : 连接套接字
 *	cliaddr : 目标地址和协议
 *	timeout : 等待时间
 *	成功返回1,超时返回0,失败返回-1
 */
int ConnectTimeout(int sockfd,struct sockaddr_in *cliaddr,int timeout)
{
	if(timeout > 0){

		NonblockFd(sockfd);
		int flag = connect(sockfd,(struct sockaddr*)cliaddr,sizeof(struct sockaddr_in));

		//connect连接未建立
		if(flag == -1 && errno == EINPROGRESS){

			fd_set rset,wset;
			FD_ZERO(&rset);
			FD_SET(sockfd,&rset);
			wset = rset;
			struct timeval t;
			t.tv_sec = timeout;
			t.tv_usec = 0;
			do{
				flag = select(sockfd + 1,&rset,&wset,NULL,&t);
			}while(flag < 0 && errno == EINTR);
			BlockFd(sockfd);
			if(flag == 0)	return 0;	//超时
			if(FD_ISSET(sockfd,&rset) || FD_ISSET(sockfd,&wset)){
				int error,len;
				getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len);
				if(error)	return -1;
				return 1;
			}
			return -1;
		}

		//连接已建立，如服务器在客户主机上
		else if(flag == 0){
			BlockFd(sockfd);
			return 1;
		}
		//出现错误
		else{
			BlockFd(sockfd);
			return -1;
		}

	}
	else{
		int flag = connect(sockfd,(struct sockaddr*)cliaddr,sizeof(struct sockaddr_in));
		if(flag < 0)	return flag;
		return 1;
	}


}




