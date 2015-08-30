#include "ftp_epoll.h"
#include "ftp_event.h"
#include "common.h"

static int epfd;
static struct epoll_event* event_list;

int ftp_epoll_init()
{
    if((epfd = epoll_create(20)) == -1)     return FTP_ERROR;
    event_list = (struct epoll_event*)malloc(20 * sizeof(struct epoll_event));
    if(event_list == NULL)  return FTP_ERROR;
    return FTP_OK;
}

int ftp_epoll_add_event(ftp_event_t* event,unsigned flag)
{
    int op;
    unsigned prev = 0;
    if(flag == FTP_READ_EVENT){
        event->read = 1;
        if(event->write) {
            op = EPOLL_CTL_MOD;
            prev = EPOLLOUT;
        }
        else
            op = EPOLL_CTL_ADD;
    }
    else{
        event->write = 1;
        if(event->read) {
            op = EPOLL_CTL_MOD;
            prev = EPOLLIN;
        }
        else
            op = EPOLL_CTL_ADD;
    }

    struct epoll_event ee;
    ee.data.ptr = event;
    ee.events = prev | flag;
    epoll_ctl(epfd,op,event->fd,&ee);

}

int ftp_epoll_solve_event()
{
    int nsize = epoll_wait(epfd,event_list,20,-1);
    int i;
    for(i = 0;i < nsize;i++) {
        ftp_event_t* peve = event_list[i].data.ptr;
        unsigned event_type = event_list[i].events;
        if(event_type & EPOLLIN)
            peve->read_handler(peve);
        else
            peve->write_handler(peve);
    }
}


