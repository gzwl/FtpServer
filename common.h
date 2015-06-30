#ifndef ROOT_H
#define ROOT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define ErrQuit(s)\
	do{\
		perror(s);\
		exit(EXIT_FAILURE);\
	}while(0)



#endif
		

