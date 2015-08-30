#include "ftp_channel.h"
#include "ftp_process.h"
#include "common.h"


static int ftp_write_channel_t(int sockfd,ftp_channel_t* ch,size_t size)
{
	struct msghdr msg;
	struct cmsghdr *pmsg;
	struct iovec vec;

	char ms[CMSG_SPACE(sizeof(ch->fd))];
	if(ch->fd != -1) {
        pmsg = CMSG_FIRSTHDR(&msg);
        pmsg->cmsg_level = SOL_SOCKET;
        pmsg->cmsg_type = SCM_RIGHTS;
        pmsg->cmsg_len = CMSG_LEN(sizeof(ch->fd));
        *(int*)CMSG_DATA(pmsg) = ch->fd;
        msg.msg_control = ms;
        msg.msg_controllen = sizeof(ms);
    }
    else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
    }

	vec.iov_base = ch;
	vec.iov_len = size;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

    return sendmsg(sockfd,&msg,0) == -1 ? FTP_ERROR : FTP_OK;
}

static int ftp_read_channel_t(int sockfd,ftp_channel_t* ch,size_t size)
{
	struct msghdr msg;
	struct cmsghdr *pmsg;
	struct iovec vec;

	char ms[CMSG_SPACE(sizeof(int))];

	vec.iov_base = ch;
	vec.iov_len = size;

	msg.msg_control = ms;
	msg.msg_controllen = sizeof(ms);

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

    if(recvmsg(sockfd,&msg,0) < 0)		return FTP_ERROR;
	pmsg = CMSG_FIRSTHDR(&msg);
	if(pmsg == NULL)    ch->fd = -1;
	else    ch->fd = *(int*)CMSG_DATA(pmsg);
	return FTP_OK;
}

int ftp_ipc_send_msg(int sockfd,int cmd,int fd)
{
    ftp_channel_t ch;
    ch.message = cmd;
    ch.slot = ftp_process_slot;
    ch.fd = fd;
    return ftp_write_channel_t(sockfd,&ch,sizeof(ch));
}

int ftp_ipc_recv_msg(int sockfd,int* cmd,int* fd)
{
    ftp_channel_t ch;
    int res = ftp_read_channel_t(sockfd,&ch,sizeof(ch));
    if(res == FTP_OK){
        *cmd = ch.message;
        if(fd)  *fd = ch.fd;
    }
    return res;
}


