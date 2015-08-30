#ifndef FTP_EPOLL_H
#define FTP_EPOLL_H

#include "ftp_event.h"
#include <sys/epoll.h>

#define FTP_READ_EVENT 10001
#define FTP_WRITE_EVENT 10002

int ftp_epoll_init();
int ftp_epoll_add_event(ftp_event_t*,unsigned);
int ftp_epoll_solve_event();

#endif
