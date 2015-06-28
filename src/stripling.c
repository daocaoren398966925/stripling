#include"stp_server.h"
#include"stp_config.h"
#include"stp_channel.h"
#include"stp_shm.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<malloc.h>
#include<signal.h>
#include<sys/wait.h>
#include<errno.h>

static stp_server_t *master_server=NULL;

void stp_signal_handler(int sig,siginfo_t *info,void *context){
	int status;
	int temp_work_process_num;
	int i;
	stp_server_t *server = master_server;
	if(NULL == server){
		printf("server NULL error \n");
		exit(1);
	}
	switch (sig){
		case SIGCHLD:
			printf("recv child exit sig\n");
			while(waitpid(-1,&status,WNOHANG)>0)
				--(server->work_process_num) ;
			break;
		case SIGQUIT:
			printf("recv quit sig \n");
			temp_work_process_num = server->work_process_num;
			for(i = 0;i<temp_work_process_num;++i){
				printf("sending quit to child %d\n",i);
				stp_socket_write(0,server->channel_socks[i],
						STP_CMD_QUITE_STR,strlen(STP_CMD_QUITE_STR));
			}
			break;
		case SIGTERM:
			printf("recv term sig \n");
			temp_work_process_num = server->work_process_num;
			for(i = 0;i<temp_work_process_num;++i){
				printf("sending term to child %d\n",i);
				stp_socket_write(0,server->channel_socks[i],
						STP_CMD_TERMINATE_SER,
						strlen(STP_CMD_TERMINATE_SER));
			}
			break;
		default:
			break;
	}
}

void store_master_pid_to_file(){
	FILE *fp = NULL;
	char file_path[100];
	strcpy(file_path,DATA_PATH);
	strcpy(file_path+strlen(file_path),"/pid.txt");
	fp = fopen(file_path,"w");
	if(NULL == fp){
		printf("stored pid error %d\n",errno);
		exit(1);
	}
	fprintf(fp,"%d",getpid());
	fclose(fp);
}

int main()
{
	sigset_t set;
	struct sigaction act;
	act.sa_sigaction = stp_signal_handler;
	//act.sa_handler = stp_signal_handler;
	act.sa_flags = SA_SIGINFO;
	sigemptyset(&act.sa_mask);
	if(sigaction(SIGCHLD,&act,NULL) == -1 || 
			sigaction(SIGQUIT,&act,NULL) == -1 ||
			sigaction(SIGTERM,&act,NULL) == -1){
		printf("sigaction ctl error exit now\n");
		exit(0);
	}
	stp_server_t server;
	if(!stp_server_init(&server)){
		printf("server init error !\n");
		return 0;
	}
	if(!stp_start_listening(&server)){
		printf("listening error !\n");
		return 0;
	}
	store_master_pid_to_file();
	master_server = &server;
	stp_start_work_process(WORKPROCESS_NUM,&server,stp_work_process);
	sigemptyset(&set);
	for(;;){
		sigsuspend(&set);
		if(0 == server.work_process_num)
			break;
	}
	stp_destroy_server(&server,0);
	printf("father exit \n");
}
