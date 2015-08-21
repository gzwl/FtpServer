#ifndef ASSIST_H
#define ASSIST_H

#include "common.h"
void CheckRoot();

void HandleSigchld();

void GetLocalIp(struct in_addr *ip);

int FileReadLock(int fd);

int FileWriteLock(int fd);

int FileUnlock(int fd);

int BlockFd(int fd);

int NonblockFd(int fd);
#endif
