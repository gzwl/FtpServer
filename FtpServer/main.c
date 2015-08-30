#include "echo.h"
#include "assist.h"
#include "configure.h"
#include "ftp_event.h"
#include "common.h"
#include "ftp_process.h"


int main(int argc,char **argv)
{
	if(getuid()){
		fprintf(stderr,"FtpServer must be started by ROOT\n");
		exit(EXIT_FAILURE);
	}
	int i;
	for(i = 0;i < max_connections;i++)  ftp_process[i].pid = -1;
	ftp_listenfd = TcpServer(listen_address,listen_port);
    ftp_master_process_cycle();
    return 0;
}

