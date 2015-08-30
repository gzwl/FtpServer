#ifndef FTP_PROCESS_CYCLE_H
#define FTP_PROCESS_CYCLE_H

#include "common.h"

void ftp_master_process_cycle();

extern sig_atomic_t ftp_sigchld;
extern sig_atomic_t ftp_reload;
extern sig_atomic_t ftp_terminate;
extern sig_atomic_t ftp_sigalrm;
extern sig_atomic_t ftp_quit;


#endif
