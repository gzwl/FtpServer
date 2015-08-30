#ifndef FTP_PROCESS_H
#define FTP_PROCESS_H

#include "common.h"
#include "ftp_event.h"

#define FTP_PROCESS_SIZE 128
#define FTP_MASTER_PROCESS 0
#define FTP_WORK_PROCESS 1

typedef struct
{
	pid_t pid;
	int sockfd[2];

	unsigned exited : 1;

}ftp_process_t;

typedef struct
{
	int signo;
	void (*handler)(int signo);
}ftp_signal_t;

void ftp_init_signal();

extern ftp_connection_t ftp_connection;
extern int ftp_process_identity;
extern int ftp_process_slot;
extern pid_t ftp_process_pid;
extern ftp_process_t ftp_process[FTP_PROCESS_SIZE];




#endif
