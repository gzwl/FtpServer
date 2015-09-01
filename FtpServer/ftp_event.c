#include "ftp_event.h"

extern ftp_connection_t ftp_connection;

ftp_event_t* ftp_event_alloc(int fd,ftp_event_handler_ptr read_handler,ftp_event_handler_ptr write_handler)
{
    ftp_event_t* pevent = (ftp_event_t*)malloc(sizeof(ftp_event_t));
    pevent->fd = fd;
    if(read_handler) {
        pevent->read = 1;
        pevent->read_handler = read_handler;
    }
    else {
        pevent->read = 0;
        pevent->read_handler = NULL;
    }

    if(write_handler) {
        pevent->write = 1;
        pevent->write_handler = write_handler;
    }
    else {
        pevent->write = 0;
        pevent->write_handler = NULL;
    }
    return pevent;
}

void ftp_event_dealloc(ftp_event_t* ptr)
{
    free(ptr);
}

void ftp_connection_init()
{
	memset(ftp_connection.command,0,sizeof(ftp_connection.command));
	memset(ftp_connection.com,0,sizeof(ftp_connection.com));
	memset(ftp_connection.args,0,sizeof(ftp_connection.args));

	ftp_connection.connfd = -1;
	ftp_connection.listenfd = -1;

	ftp_connection.login = -1;
	ftp_connection.useruid = -1;
	memset(ftp_connection.username,0,sizeof(ftp_connection.username));

	ftp_connection.pasv = 0;
	ftp_connection.port = 0;

  	ftp_connection.transmode = 0;

	ftp_connection.restart_pos = 0;

	ftp_connection.addr = NULL;

    ftp_connection.nobody = NULL;
	ftp_connection.client = NULL;
}
