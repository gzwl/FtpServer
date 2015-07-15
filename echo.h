#ifndef ECHO_H
#define ECHO_H

#include "common.h"

ssize_t readn(int fd,void *buf,size_t n);
ssize_t writen(int fd,void *buf,size_t n);
ssize_t readline(int fd,void *buf,size_t maxlen);
int TcpServer(const char *host,unsigned short port);
int SendFd(int des,int fd);
int RecvFd(int des,int *fd);
int AcceptTimeout(int sockfd,struct sockaddr_in *cliaddr,int timeout);
int ConnectTimeout(int sockfd,struct sockaddr_in *cliaddr,int timeout);

#endif
