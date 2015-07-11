#ifndef ASSIST_H
#define ASSIST_H

#include "common.h"
void CheckRoot();

void HandleSigchld();

void GetLocalIp(struct in_addr *ip);
#endif
