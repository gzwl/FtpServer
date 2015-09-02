#include "ftp_process.h"
#include "configure.h"
#include "ftp_process_cycle.h"
#include "ftp_channel.h"
#include "ftp_epoll.h"
#include "ftp_event.h"
#include "ftp_command.h"

#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

sig_atomic_t ftp_sigchld;
sig_atomic_t ftp_sigalrm;
sig_atomic_t ftp_reload;
sig_atomic_t ftp_terminate;
sig_atomic_t ftp_quit;



static pthread_mutex_t *mptr;

static int ftp_respawn_work_process(int);
static void ftp_work_process_init();
static void ftp_work_process_cycle();
static void ftp_signal_work_process(int signo);
static int ftp_work_process_cmd(ftp_event_t*);
static void ftp_nobody_process_cycle();
static void ftp_nobody_process_init();
static void ftp_nobody_listenfd();
static void ftp_nobody_accept_data();
static void ftp_nobody_connect_data();

void ftp_master_process_cycle()
{
    int fd = open("/dev/zero",O_RDWR,0);
    mptr = mmap(0,sizeof(pthread_mutex_t),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    close(fd);

    ftp_process_identity = FTP_MASTER_PROCESS;

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr,PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(mptr,&mattr);

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set,SIGCHLD);
	sigaddset(&set,SIGINT);
	sigaddset(&set,SIGTERM);
	sigaddset(&set,SIGALRM);
	if(sigprocmask(SIG_SETMASK,&set,NULL) == 1){
		err_quit("ftp_master_process_cycle - sigprocmask");
	}

    int i;
    for(i = 0;i < max_connections;i++)
        ftp_respawn_work_process(i);

    close(ftp_listenfd);

    sigemptyset(&set);

    unsigned delay = 0;
	while(1) {
		sigsuspend(&set);
		if(ftp_sigchld) {
            ftp_sigchld = 0;
            int alive = 0 ;
            for(i = 0;i < max_connections;i++) {
                if(ftp_process[i].pid == -1 && !ftp_process[i].exited) {
                    ftp_respawn_work_process(i);
                }
                if(ftp_process[i].pid)  alive = 1;
            }
            if(!alive && (ftp_terminate || ftp_quit))   exit(0);
		}

        if(ftp_terminate) {
            if(delay == 0) {
                delay = 2000;
                struct itimerval t;
                t.it_interval.tv_sec = 0;
                t.it_interval.tv_usec = 0;
                t.it_value.tv_sec = delay / 1000;
                t.it_value.tv_usec = delay % 1000 * 1000;
                setitimer(ITIMER_REAL,&t,NULL);
            }
            ftp_signal_work_process(SIGTERM);
        }

        if(ftp_sigalrm) {
            ftp_signal_work_process(SIGKILL);
        }

	}

}

static int ftp_respawn_work_process(int slot)
{
    if(ftp_process[slot].pid == -1 && !ftp_process[slot].exited) {
        if(socketpair(AF_UNIX,SOCK_DGRAM,0,ftp_process[slot].sockfd) < 0){
            err_quit("socketpair");
        }

        int on = 1;
        if(ioctl(ftp_process[slot].sockfd[1],FIONBIO,&on) == -1) {
            err_quit("ioctl");
        }

        pid_t pid = fork();
        if(pid) {
            ftp_process[slot].pid = pid;
            close(ftp_process[slot].sockfd[1]);
        }
        else {
            ftp_process_slot = slot;
            ftp_process_pid = pid;
            close(ftp_process[slot].sockfd[0]);
            ftp_work_process_cycle();
        }
        return FTP_OK;
    }
    return FTP_ERROR;
}

void ftp_signal_work_process(int signo)
{
    int i;
    for(i = 0;i < max_connections;i++) {
        if(ftp_process[i].pid != -1) {
            ftp_process[i].exited = 1;
            if(signo == SIGKILL)   kill(ftp_process[i].pid,SIGKILL);
            else    ftp_ipc_send_msg(ftp_process[i].sockfd[0],signo,-1);
        }
    }
}

/*
    work process
*/
void ftp_work_process_cycle()
{
    ftp_work_process_init();

    while(pthread_mutex_lock(mptr)) {
        if(errno == EINTR)  continue;
        else err_quit("pthread_mutex_lock");
    }

    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    while((ftp_connection.connfd = accept(ftp_listenfd,(struct sockaddr*)&cliaddr,&len)) == -1)
            continue;

    int on = 1;
    if(ioctl(ftp_connection.connfd,FIONBIO,&on) == -1) {
        err_quit("ioctl");
    }
    close(ftp_listenfd);
    pthread_mutex_unlock(mptr);

    if(ftp_epoll_init() != FTP_OK) {
        err_quit("ftp_epoll_init");
    }

    ftp_connection.client = ftp_event_alloc(ftp_connection.connfd,ftp_request_handler,NULL);

    ftp_reply(FTP_SERVER_READY,"FtpServer1.0\r\n");

    if(ftp_epoll_add_event(ftp_connection.client,FTP_READ_EVENT) != FTP_OK) {
        err_quit("ftp_epoll_add_event");
    }

    ftp_event_t* pevent = ftp_event_alloc(ftp_process[ftp_process_slot].sockfd[1],ftp_work_process_cmd,NULL);

    if(ftp_epoll_add_event(pevent,FTP_READ_EVENT) != FTP_OK) {
        err_quit("ftp_epoll_add_event");
    }

    while(1){
        if(ftp_reload || ftp_quit || ftp_terminate) {
            exit(0);
        }
        ftp_epoll_solve_event();
    }
}

void ftp_work_process_init()
{
    ftp_process_identity = FTP_WORK_PROCESS;

    int i;
    for(i = 0;i < max_connections;i++) {
        if(ftp_process[i].pid != -1)
            close(ftp_process[i].sockfd[0]);
    }

    sigset_t set;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK,&set,NULL);

	ftp_connection_init();

	//创建nobody进程
	int fd[2];
	if((socketpair(AF_UNIX,SOCK_STREAM,0,fd)) < 0){
			err_quit("socketpair");
	}

	pid_t pid;
	if((pid = fork()) < 0){
		err_quit("fork");
	}
	else if(pid == 0){
        close(fd[0]);
        ftp_nobody_process_cycle(fd[1]);
	}

    int on = 1;
    if(ioctl(fd[0],FIONBIO,&on) == -1) {
        err_quit("ioctl");
    }

    ftp_connection.nobodyfd = fd[0];
    ftp_connection.nobody = ftp_event_alloc(fd[0],NULL,NULL);

	close(fd[1]);

}

int ftp_work_process_cmd(ftp_event_t* ptr)
{
    int cmd;
    if(ftp_ipc_recv_msg(ptr->fd,&cmd,NULL) != FTP_OK)   return FTP_ERROR;
    switch(cmd) {
        case SIGTERM :
        case SIGINT : ftp_terminate = 1;break;
        case SIGQUIT : ftp_quit = 1;break;
    }
    return FTP_OK;
}


/*
    nobody process
*/

void ftp_nobody_process_cycle(int fd)
{
    int listenfd = -1;
    ftp_nobody_process_init();
	int cmd;
	while(1){
		if(ftp_ipc_recv_msg(fd,&cmd,NULL) != FTP_OK)	exit(0);
		switch(cmd){
				case IPC_LISTEN_OPEN : ftp_nobody_listenfd(fd,&listenfd);break;
				case IPC_ACCEPT		 : ftp_nobody_accept_data(fd,listenfd);break;
				case IPC_CONNECT	 : ftp_nobody_connect_data(fd,listenfd);break;
		}
	}
}

void ftp_nobody_process_init()
{
	//关闭多余的文件描述符
	close(ftp_connection.connfd);
	close(ftp_listenfd);

	//设置为nobody进程
	struct passwd *p;
    if((p = getpwnam("nobody")) == NULL){
		err_quit("getpwnam");
	}
	if(setegid(p->pw_gid) == -1){
		err_quit("NobodyInit setegid");
	}
	if(seteuid(p->pw_uid) == -1){
		err_quit("NobodyInit seteuid");
	}

	//为进程添加绑定20端口的权限
	struct __user_cap_header_struct cap_user_header;
	cap_user_header.version = _LINUX_CAPABILITY_VERSION_2;
 	cap_user_header.pid = getpid();
	struct __user_cap_data_struct cap_user_data;
	__u32 cap_mask = 0; 						//类似于权限的集合
	cap_mask |= (1 << CAP_NET_BIND_SERVICE); 	//0001000000
	cap_user_data.effective = cap_mask;
	cap_user_data.permitted = cap_mask;
	cap_user_data.inheritable = 0; 				//子进程不继承特权
	if(capset(&cap_user_header, &cap_user_data) == -1)
			err_quit("capset");
}


//打开监听套接字，PASV模式下使用
static void ftp_nobody_listenfd(int fd,int* listenfd)
{
    if(*listenfd == -1){
        struct in_addr ip;
        ftp_get_local_ip(&ip);
        char ipstr[40] = {0};
        inet_ntop(AF_INET,&ip,ipstr,sizeof(ipstr));
        *listenfd = TcpServer(ipstr,20);
    }
	if(*listenfd == -1)
		ftp_ipc_send_msg(fd,FTP_OK,-1);
	else
		ftp_ipc_send_msg(fd,FTP_ERROR,-1);
}

//PASV模式建立数据连接
static void ftp_nobody_accept_data(int fd,int listenfd)
{
	int datafd = AcceptTimeout(listenfd,NULL,data_timeout);
	if(datafd <= 0){
		ftp_ipc_send_msg(fd,FTP_ERROR,-1);
	}
	ftp_ipc_send_msg(fd,FTP_OK,datafd);
	close(datafd);
}

//PORT模式建立数据连接
static void ftp_nobody_connect_data(int fd)
{
	struct sockaddr_in cliaddr;
	memset(&cliaddr,0,sizeof(&cliaddr));
	cliaddr.sin_family = AF_INET;
	ftp_ipc_recv_msg(fd,&cliaddr.sin_addr.s_addr,NULL);
	ftp_ipc_recv_msg(fd,&cliaddr.sin_port,NULL);
	int datafd = socket(AF_INET,SOCK_STREAM,0);
	if(ConnectTimeout(datafd,&cliaddr,data_timeout) <= 0){
		ftp_ipc_send_msg(fd,FTP_ERROR,-1);
		return ;
	}
	ftp_ipc_send_msg(fd,FTP_OK,datafd);
	close(datafd);
}

