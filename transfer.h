#ifndef TRANSFER_H
#define TRANSFER_H

#include "event.h"

void DownloadFile(event_t *ptr);

void UploadFile(event_t *ptr);

void SendList(event_t *ptr,int op);

#endif
