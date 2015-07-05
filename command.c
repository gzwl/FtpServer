#include "command.h"
#include "event.h"
#include "common.h"

void FtpReply(event_t *ptr,int status,const char* text)
{
	char buf[1024] = {0};
	snprintf(buf,sizeof(buf),"%d %s\r\n",status,text);
	Write(ptr->connfd,buf,strlen(buf));
}
