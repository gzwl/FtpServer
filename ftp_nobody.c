#include "ftp_nobody.h"

void NobodyInit(event_t *ptr)
{
	//关闭多余的文件描述符
	close(ptr->connfd);
	close(ptr->workfd);
	ptr->workfd = -1;

	//设置为nobody进程
	struct passwd *p;
    if((p = getpwnam("nobody")) == NULL){
		ErrQuit("getpwnam");
	}
	if(seteuid(p->pw_uid) == -1){
		ErrQuit("seteuid");
	}
	if(setegid(p->pw_gid) == -1){
		ErrQuit("setegid");
	}

	//为进程添加绑定20端口的权限
	struct __user_cap_header_struct cap_user_header;
	cap_user_header.version = _LINUX_CAPABILITY_VERSION_2;
 	cap_user_header.pid = getpid();
	struct __user_cap_data_struct cap_user_data;
	__u32 cap_mask = 0; 						//类似于权限的集合
	cap_mask |= (1 << CAP_NET_BIND_SERVICE); 	//0001000000
	cap_user_data.effective = cap_mask;
	cap_user_data.permitted = cap_mask;
	cap_user_data.inheritable = 0; 				//子进程不继承特权
	if(capset(&cap_user_header, &cap_user_data) == -1)
			ERR_EXIT("capset");	
	
}

void NobodyHandle(event_t *ptr)
{

}
