#ifndef ECHO_H
#define ECHO_H

#include "common.h"

ssize_t readn(int fd,void *buf,size_t n);
ssize_t writen(int fd,void *buf,size_t n);
ssize_t readline(int fd,void *buf,size_t maxlen);

#endif
