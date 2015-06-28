#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include"stp_config.h"

void print_help(){
	printf("commond format : stp_cmd -option \n");
	printf("option : \n");
	printf("    quit ---- quit the server." 
			"all process will be exit nomarlly\n");
	printf("    term ---- term the server. all "
			"process will be terminated immediately\n");
}

int read_pid_from_file(){
	FILE *fp = NULL;
	int pid;
	int res;
	char file_path[100];
	strcpy(file_path,DATA_PATH);
	strcpy(file_path+strlen(file_path),"/pid.txt");
	fp = fopen(file_path,"r");
	if( NULL == fp ){
		printf("open pid file error \n");
		exit(1);
	}
	res = fscanf(fp,"%d",&pid);
	if(1 != res){
		printf("read pid error %d\n",errno);
		//perror("error");
		close(fp);
		exit(1);
	}
	close(fp);
	return pid;
}

int main(int argc,char *argv[])
{
	int pid;
	int res;
	char option[100];
	if(argc <= 1 || argc > 2){
		print_help();
		return 1;
	}
	strcpy(option,argv[1]);
	if(!( (0 == strcmp(option,"-quit")) 
				|| (0 == strcmp(option,"-term")) )){
		print_help();
		return 1;
	}
	pid = read_pid_from_file();
	if(0 == strcmp(option,"-quit")){
		res = kill(pid,SIGQUIT);
		if( -1 == res ){
			printf("send QUIT sig error \n");
			return 0;
		}
	}else{
		res = kill(pid,SIGTERM);
		if( -1 == res ){
			printf("send TERM sig error \n");
			return 0;
		}
	}
	return 1;
}
