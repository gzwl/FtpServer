#ifndef FTP_COMMAND_H
#define FTP_COMMAND_H

#include "ftp_event.h"

# define FTP_DATA_OK			150

# define FTP_COMMAND_SUCCESS	200
# define FTP_PORT_OK			200
# define FTP_COMMAND_FAIL		202
# define FTP_SIZE_OK			213
# define FTP_SERVER_READY 	 	220
# define FTP_DATA_OVER_CLOSE    226
# define FTP_PASV_OK			227
# define FTP_LOGIN_SUCCESS		230
# define FTP_FILE_SUCCESS		250
# define FTP_PWD_OK				257

# define FTP_USERNAME_OK   		331
# define FTP_REST_OK			350

# define FTP_CONTROL_CLOSE 		421
# define FTP_DATA_BAD			425
# define FTP_OVER				426
# define FTP_FILE_FAIL 			450

# define FTP_COMMAND_ERR   		500
# define FTP_PARAMETER_BAD		501
# define FTP_COMMAND_SEQ   		503
# define FTP_LOGIN_ERR     		530
# define FTP_FILE_ERR			550
void ftp_reply(int status,const char *text);

int ftp_request_handler(ftp_event_t *ptr);

#endif
