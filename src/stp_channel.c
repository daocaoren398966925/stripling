#include"stp_channel.h"
#include<sys/socket.h>
#include<sys/types.h>
#include<stdio.h>
#include<string.h>

int stp_init_socketpair(int socket[2]){
	if(-1 != socketpair(AF_UNIX,SOCK_STREAM,0,socket))
		return 1;
	return 0;
}

void stp_socket_close(int is_work_pro,int socket[2]){
	if(is_work_pro)
		close(socket[0]);
	else
		close(socket[1]);
}

int stp_socket_write(int is_work_pro,int socket[2],void*buf,int size){
	int fd = socket[0];
	if(is_work_pro)
		fd = socket[1];
	return write(fd,buf,size);
}

int stp_socket_read(int is_work_pro,int socket[2],void*buf,int size){
	int fd = socket[0];
	if(is_work_pro)
		fd = socket[1];
	return read(fd,buf,size);
}

int stp_socket_cmd_resolve(char* cmd){
	int len;
	int int_cmd;
	if(NULL == cmd)
		return 0;
	len = strlen(cmd);
	if(len > 1)
		return 0;
	int_cmd = cmd[0]-'0';
	return int_cmd;
}
