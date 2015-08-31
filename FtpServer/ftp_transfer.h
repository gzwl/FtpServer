#ifndef TRANSFER_H
#define TRANSFER_H

#include "ftp_event.h"

#define FTP_CMD_RETR 0
#define FTP_CMD_APPE 1
#define FTP_CMD_STOR 2
#define FTP_CMD_LIST 3
#define FTP_CMD_NLST 4

int ftp_get_data_fd(ftp_event_t *ptr,int op);


#endif
