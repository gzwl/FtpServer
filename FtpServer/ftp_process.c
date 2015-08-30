#include "ftp_process_cycle.h"
#include "ftp_process.h"
#include "configure.h"

ftp_connection_t ftp_connection;
int ftp_process_identity;
int ftp_process_slot;
pid_t ftp_process_pid;
ftp_process_t ftp_process[FTP_PROCESS_SIZE];

static void ftp_signal_handler(int signo);
static void ftp_get_children_status();

ftp_signal_t ftp_signal[] =
{
	{SIGCHLD,	ftp_signal_handler},
	{SIGINT,	ftp_signal_handler},
	{SIGTERM,   ftp_signal_handler},
	{SIGHUP,	ftp_signal_handler},
	{SIGALRM,   ftp_signal_handler},
	{SIGQUIT,   ftp_signal_handler},
	{0,			NULL}
};

void ftp_init_signal()
{
	ftp_signal_t *sig;
	while(sig->signo){
		signal(sig->signo,sig->handler);
		++sig;
	}
}

void ftp_signal_handler(int signo)
{
    if(ftp_process_identity == FTP_MASTER_PROCESS) {
        switch(signo) {
            case SIGCHLD : ftp_sigchld = 1; break;
            case SIGINT :
            case SIGTERM : ftp_terminate = 1; break;
            case SIGQUIT : ftp_quit = 1; break;
            case SIGHUP : ftp_reload = 1; break;
            case SIGALRM : ftp_sigalrm = 1; break;
        }
        if(ftp_sigchld)
            ftp_get_children_status();
    }
    else {
        switch(signo) {
            case SIGCHLD : ftp_sigchld = 1; break;
            case SIGINT :
            case SIGTERM : ftp_terminate = 1; break;
            case SIGQUIT : ftp_quit = 1; break;
            case SIGHUP : ftp_reload = 1; break;
        }
        if(ftp_sigchld)  wait(NULL);
    }
}

void ftp_get_children_status()
{
    int status;
    while(1) {
        pid_t pid = waitpid(-1,&status,WNOHANG);
        if(pid == 0)    return ;
        if(pid == -1) {
            if(errno == EINTR)  continue;
            else    return ;
        }
        unsigned i;
        for(i = 0;i < max_connections;i++) {
            if(ftp_process[i].pid == pid) {
                ftp_process[i].pid = -1;
                break;
            }
        }
    }
}




