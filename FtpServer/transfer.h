#ifndef TRANSFER_H
#define TRANSFER_H

#include "ftp_event.h"

void download_file(ftp_event_t *ptr);

void upload_file(ftp_event_t *ptr,int op);

void send_list(ftp_event_t *ptr,int op);

#endif
