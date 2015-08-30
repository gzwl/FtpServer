#ifndef COMMON_H
#define COMMON_H

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
#include <netdb.h>
#include <linux/capability.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <shadow.h>
#include <crypt.h>
#include <dirent.h>
#include <time.h>


#define err_quit(s)\
	do{\
			perror(s);\
			exit(EXIT_FAILURE);\
	}while(0)

#define FTP_OK 0
#define FTP_ERROR -1
int ftp_listenfd;

#endif
