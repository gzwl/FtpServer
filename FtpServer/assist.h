#ifndef ASSIST_H
#define ASSIST_H

#include "common.h"
void CheckRoot();

void HandleSigchld();

void ftp_get_local_ip(struct in_addr *ip);

int ftp_file_read_lock(int fd);

int ftp_file_write_lock(int fd);

int ftp_file_unlock(int fd);

int BlockFd(int fd);

int NonblockFd(int fd);
#endif
