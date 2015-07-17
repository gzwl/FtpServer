#include "transfer.h"
#include "common.h"
#include "command.h"
#include "work.h"
#include "assist.h"

//#define TEST
static int GetDatafd(event_t *ptr);
static char* GetListName(struct stat *buf,const char* name);
static char* GetListSize(struct stat *buf);
static char* GetListInfo(struct stat *buf);
static char* GetListType(struct stat *buf);
static char* GetListTime(struct stat *buf);

void DownloadFile(event_t *ptr)
{
	//打开文件	
	int fd = open(ptr->args,O_RDONLY);
	if(fd <  0){
		FtpReply(ptr,FTP_FILE_FAIL,"Open file fail\r\n");
		return ;
	}

	//对文件加读锁
	if(FileReadLock(fd) < 0){
		FtpReply(ptr,FTP_FILE_FAIL,"Open file fail\r\n");
		return ;
	}
	struct stat file;
	if(fstat(fd,&file) < 0){
		FileUnlock(fd);
		FtpReply(ptr,FTP_FILE_FAIL,"Open file fail\r\n");
		return ;
	}

	//只能传递普通文件
	if(!S_ISREG(file.st_mode)){
		FileUnlock(fd);
		FtpReply(ptr,FTP_FILE_FAIL,"Can only download regular file\r\n");
		return ;
	}

	//建立数据连接
	if(GetDatafd(ptr) < 0){
		FileUnlock(fd);
		return ;
	}
    
	size_t filesize = file.st_size;
	filesize -= ptr->restart_pos;
	lseek(fd,ptr->restart_pos,SEEK_SET);
	ptr->restart_pos = 0;

	char text[64];
	if(ptr->transmode == 0){
		sprintf(text,"Begin to transfer the file in ASCII mode(%d bytes)",filesize);
	}
	else
		sprintf(text,"Begin to transfer the file in BINARY mode(%d bytes)",filesize);
	FtpReply(ptr,FTP_DATA_OK,text);


	while(filesize > 0){
		size_t nwrite = sendfile(ptr->datafd,fd,NULL,1024);
		if(nwrite == -1 && errno == EINTR)	continue;
		filesize -= nwrite;
	}
	
	FileUnlock(fd);
	close(fd);
	close(ptr->datafd);
	FtpReply(ptr,FTP_DATA_OVER_CLOSE,"Download file success\r\n");

}

void SendList(event_t *ptr,int op)
{
	if(GetDatafd(ptr) < 0){
		return ;
	}
    DIR *dir = opendir(".");
	if(dir == NULL){
		FtpReply(ptr,FTP_COMMAND_FAIL,"Can not open directory");
		return ;
	}
	FtpReply(ptr,FTP_DATA_OK,"Here comes the directory list");

	//LIST方式
	if(op == 0){

#ifdef TEST
	printf("Enter LIST option\n");
#endif
		struct dirent *dp;	
		while(dp = readdir(dir)){
			if(dp->d_name[0] == '.')		continue;
			char text[1024] = {0};
			struct stat buf;
			if(lstat(dp->d_name,&buf) < 0)
					ErrQuit("SendList - lstat");
			strcpy(text,GetListType(&buf));
			strcat(text," ");
			strcat(text,GetListInfo(&buf));
			strcat(text," ");
			strcat(text,GetListSize(&buf));
			strcat(text," ");
			strcat(text,GetListTime(&buf));
			strcat(text," ");
			strcat(text,GetListName(&buf,dp->d_name));
			strcat(text,"\r\n");
#ifdef TEST
	printf("%s",text);
#endif		
			Write(ptr->datafd,text,strlen(text));
		}

	}

	//NLST方式
	else{

#ifdef TEST
	printf("Enter NLST option");
#endif
		struct dirent *dp;
	    while(dp = readdir(dir)){
			if(dp->d_name[0] == '.')		continue;
	   		char text[1024] = {0};
	   		struct stat buf;
		    if(lstat(dp->d_name,&buf) < 0)
					ErrQuit("SendList - lstat");
			strcpy(text,GetListName(&buf,dp->d_name));
			strcat(text,"\r\n");
#ifdef TEST
	printf("%s",text);
#endif
			Write(ptr->datafd,text,strlen(text));	
	    }	
	}

	closedir(dir);
	close(ptr->datafd);
	FtpReply(ptr,FTP_DATA_OVER_CLOSE,"Send directory completely");
}



static int GetDatafd(event_t *ptr)
{
	if(!ptr->pasv && !ptr->port){
		FtpReply(ptr,FTP_COMMAND_FAIL,"Please designated the work mode(PASV/PORT)\r\n");
		return -1;
	}

	//PASV模式
	if(ptr->pasv){
		IpcSendCommand(ptr,IPC_ACCEPT);
		char res;
		IpcRecvResult(ptr,&res);
		if(res != IPC_COMMAND_OK){
			FtpReply(ptr,FTP_DATA_BAD,"Can not found data connection\r\n");
			return -1;
		}
		if(IpcRecvFd(ptr,&ptr->datafd) < 0){
			FtpReply(ptr,FTP_DATA_BAD,"Can not found data connection\r\n");
			return -1;
		}	
	}

	//PORT模式
	else{
		IpcSendCommand(ptr,IPC_CONNECT);
		IpcSendDigit(ptr,ptr->addr->sin_addr.s_addr);
		IpcSendDigit(ptr,ptr->addr->sin_port);
		char res;
		IpcRecvResult(ptr,&res);
		if(res == IPC_COMMAND_BAD){
			FtpReply(ptr,FTP_DATA_BAD,"Can not found data connection\r\n");
			return -1;
		}
		if(IpcRecvFd(ptr,&ptr->datafd) < 0){
			FtpReply(ptr,FTP_DATA_BAD,"Can not found data connection\r\n");
			return -1;
		}
	}

	ptr->pasv = 0;
	if(ptr->port){
		ptr->port = 0;
		free(ptr->addr);
	}

	return 0;
}


static char* GetListName(struct stat *pstat,const char *name)
{
	static char filename[128] = {0};
	if(S_ISLNK(pstat->st_mode)){
		char buf[64];
		readlink(name,buf,sizeof(buf));
		sprintf(filename,"%s -> %s",name,buf);
	}
	else
		strcpy(filename,name);
	return filename;
}

static char* GetListType(struct stat *pstat)
{
	static char filetype[] = "----------";
	
	if(S_ISREG(pstat->st_mode)) 		filetype[0] = '-';
	else if(S_ISDIR(pstat->st_mode))	filetype[0] = 'd';
	else if(S_ISCHR(pstat->st_mode))	filetype[0] = 'c';
	else if(S_ISBLK(pstat->st_mode))	filetype[0] = 'b';
	else if(S_ISFIFO(pstat->st_mode))	filetype[0] = 'p';
	else if(S_ISLNK(pstat->st_mode))	filetype[0] = 'l';
	else if(S_ISSOCK(pstat->st_mode))	filetype[0] = 's';

	filetype[1] = (pstat->st_mode & S_IRUSR) ? 'r' : '-';
	filetype[2] = (pstat->st_mode & S_IWUSR) ? 'w' : '-';
	filetype[3] = (pstat->st_mode & S_IXUSR) ? 'x' : '-';

	filetype[4] = (pstat->st_mode & S_IRGRP) ? 'r' : '-';
	filetype[5] = (pstat->st_mode & S_IWGRP) ? 'w' : '-';
	filetype[6] = (pstat->st_mode & S_IXGRP) ? 'x' : '-';

	filetype[7] = (pstat->st_mode & S_IROTH) ? 'r' : '-';
	filetype[8] = (pstat->st_mode & S_IWOTH) ? 'w' : '-';
	filetype[9] = (pstat->st_mode & S_IXOTH) ? 'x' : '-';
	
	return filetype;
}

static char* GetListSize(struct stat *pstat)
{
	static char filesize[16];
	sprintf(filesize,"%8u",(unsigned)pstat->st_size);
	return filesize;

}

static char* GetListInfo(struct stat *pstat)
{
	static char fileinfo[128];
	sprintf(fileinfo," %3d %8d %8d",pstat->st_nlink,pstat->st_uid,pstat->st_gid);
	return fileinfo;
}

static char* GetListTime(struct stat *pstat)
{
	static char filetime[128];
	time_t tsec = pstat->st_ctime;
	struct tm *p = localtime(&tsec);
	if(p == NULL){
		ErrQuit("GetListTime - localtime");
	}
	strftime(filetime,sizeof(filetime),"%b %e %H:%M",p);
	return filetime;
}







