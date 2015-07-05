#ifndef COMMAND_H
#define COMMAND_H

#include "event.h"

# define FTP_SERVER_READY 220
# define FTP_CONTROL_CLOSE 421

void FtpReply(event_t *ptr,int status,const char *text);



#endif
