#include"stp_server.h"
#include"stp_channel.h"
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<string.h>
#include<signal.h>
#include<sys/wait.h>
#define __USE_GNU
#include<sched.h>

int stp_work_process(stp_server_t* server){
	stp_creat_epoll_and_init(server);
	//stp_channel_handler(server);
	stp_epoll_circle(server);
	/*
	char buff[1000];
	int cmd;
	cmd = stp_socket_read(1,
			server->channel_socks[server->process_index],
			buff,1000);
	printf("recv size : %d\n",cmd);
	printf("child recved : %s\n",buff);
	cmd = stp_socket_cmd_resolve(buff);
	stp_channel_handler_core(cmd,server);
	*/
}

int stp_start_work_process(int process_num,stp_server_t* server,
		stp_work_process_circle work_fun){
	int i;
#ifdef CPU_BOUND
	cpu_set_t mask;
#endif

#ifndef MASTER_MODEL
	for(i = 0;i<process_num;++i){
		pid_t pid;
		if(!stp_init_socketpair(server->channel_socks[i]))
			return 0;
		pid = fork();
		if(pid < 0){
			return 0;
		}else if(pid == 0){
			printf(" work process start %d\n",getpid());
			server->process_index = i;
			stp_socket_close(1,server->channel_socks[i]);
			work_fun(server);
			stp_destroy_server(server,1);
			printf(" work process exit !\n");
			exit(1);
		}
#ifdef CPU_BOUND
		CPU_ZERO(&mask);
		CPU_SET(i,&mask);
		if(-1 == sched_setaffinity(pid,sizeof(mask),&mask)){
			printf("cpu setaffinity failed\n");
		}
#endif
		stp_socket_close(0,server->channel_socks[i]);
		printf("i am father %d\n",getpid());
		//printf("father sending quit\n");
		//stp_socket_write(0,server->channel_socks[i],STP_CMD_QUITE_STR,
		//		strlen(STP_CMD_QUITE_STR));
	}
	server->process_index = -1;
	close(server->listenfd);
#else
	printf("starting work_fun()\n");
	work_fun(server);
#endif
	return 1;
}

bool stp_server_init(stp_server_t* server){
	int i;
	stp_connection_t* curr;
	stp_event_t* curr_event;
	if(NULL == server)
		return false;
	server->pool = stp_creat_pool(1024*4);
	if(NULL == server->pool)
		return false;
	server->listen_block_length = LISTEN_BLOCK_LENGTH;
	server->total_num_of_connections = MAX_CONNECTION_NUM;
	server->num_of_free_connections = MAX_CONNECTION_NUM;
#ifndef MASTER_MODEL
	server->work_process_num = CPU_NUM;
	server->channel_read_index = 0;
	server->disable_to_accept = MAX_CONNECTION_NUM/8 - MAX_CONNECTION_NUM;
	stp_list_init(&server->post_free_event_list);
	stp_list_init(&server->post_used_event_list);
#endif
	stp_list_init(&server->free_connections);
	stp_list_init(&server->used_connections);
	for(i = 0;i<MAX_CONNECTION_NUM;++i){
		curr = (stp_connection_t*)stp_pool_alloc(
				server->pool,sizeof(stp_connection_t));
		/*
		curr->pool = NULL;
		curr->sockfd = 0;
		curr->request.file_fd = 0;
		curr->request.file_len = 0;
		curr->request.read_index = 0;
		curr->request.response_code = -1;
		curr->request.status = -1;
		curr->request.total_length_to_write = 0;
		curr->request.write_index = 0;
		*/
#ifndef MASTER_MODEL
		curr_event = (stp_event_t*)stp_pool_alloc(server->pool,
				sizeof(stp_event_t));
		if(NULL == curr || NULL == curr_event){
			stp_destroy_pool(server->pool);
			return false;
		}
#else
		if(NULL == curr){
			stp_destroy_pool(server->pool);
			return false;
		}
#endif
		stp_add_list_at_tail(&server->free_connections,&curr->list);
#ifndef MASTER_MODEL
		stp_add_list_at_tail(&server->post_free_event_list,
				&curr_event->list);
#endif
	}
#ifndef MASTER_MODEL
	if(!stp_shmtx_create(&server->accpet_mutx))
		return false;
	server->is_hold_accpet_mutx = 0;
#endif
	return true;
}

void stp_destroy_server(stp_server_t* server,int is_worker){
	close(server->listenfd);
	stp_destroy_pool(server->pool);
#ifndef MASTER_MODEL
	if(!is_worker)
		stp_shmtx_destory(&server->accpet_mutx);
#endif
}

bool stp_start_listening(stp_server_t* server){
	server->port = SERVER_PORT;
	server->addr_in.sin_family = AF_INET;
	server->addr_in.sin_port = htons(SERVER_PORT);
	server->addr_in.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server->addr_in.sin_zero),0);
	server->listenfd = socket(AF_INET,SOCK_STREAM,0);
	if(-1 == server->listenfd)
		return false;
	if(-1 == setNonblocking(server->listenfd))
		return 0;
	if(bind(server->listenfd,(struct sockaddr*)&server->addr_in,
				sizeof(struct sockaddr)) == -1){
#ifdef MASTER_MODEL
		printf("bind error %d\n",errno);
#endif
		return false;
	}
	if(listen(server->listenfd,server->listen_block_length) == -1){
#ifdef MASTER_MODEL
		printf("listen error %d\n",errno);
#endif
		return false;
	}
#ifdef MASTER_MODEL
	printf("server listening ok\n");
#endif
	return true;
}

int stp_creat_epoll_and_init(stp_server_t *server){
	struct epoll_event ev;
	int res;
	server->epfd = epoll_create1(0);
	if(res == -1)
		return 0;
#ifndef MASTER_MODEL
	if(server->process_index >= 0){
		if(-1 == setNonblocking(server->channel_socks[server->process_index][1]))
			return 0;
		ev.data.fd = server->channel_socks[server->process_index][1];
		ev.events = EPOLLIN | EPOLLET;
		res = epoll_ctl(server->epfd,EPOLL_CTL_ADD,
				server->channel_socks[server->process_index][1],
				&ev);
		if(res == -1)
			return 0;
	}
#else
	ev.data.fd = server->listenfd;
	ev.events = EPOLLIN | EPOLLET;
	epoll_ctl(server->epfd,EPOLL_CTL_ADD,server->listenfd,&ev);
	printf("epoll add listen done\n");
#endif
	return 1;
}

int stp_accept_handler(stp_server_t *server){
	int listenfd = server->listenfd;
	struct sockaddr addr_in;
	socklen_t len;
	int connectionfd;
	struct epoll_event ev;
	stp_list_t* connection_node;
	stp_connection_t* connection;
	int res;
	while(1){
		if(server->num_of_free_connections == 0){
#ifdef MASTER_MODEL
			printf("server->num_of_free_connections == 0\n");
#endif
			len = sizeof(addr_in);
			connectionfd = accept(listenfd,&addr_in,&len);
			if(connectionfd == -1){
				if(EINTR == errno || EWOULDBLOCK == errno || 
						EAGAIN == errno)
					break;
				else{
					printf("accept error !\n");
					break;
				}
			}else
				close(connectionfd);
		}else{
			len = sizeof(addr_in);
			connectionfd = accept(listenfd,&addr_in,&len);
			if(connectionfd == -1){
				if(EINTR == errno || EWOULDBLOCK == errno || 
						EAGAIN == errno){
#ifndef MASTER_MODEL
					res = epoll_ctl(server->epfd,EPOLL_CTL_DEL,
							server->listenfd,&ev);
					if(-1 == res){
#ifdef DEBUG
						printf("del listenfd error\n");
#endif
						return 0;
					}else
#ifdef DEBUG
						printf("del listenfd done\n");
#endif

#endif
					return 1;
				}else{
					printf("accept error ! %d\n",errno);
					return 0;
				}
			}else{
				if(!setNonblocking(connectionfd))
					return 0;
				connection = stp_get_connection(server);

#ifndef MASTER_MODEL
				server->disable_to_accept = 
					server->total_num_of_connections/8 - 
					server->num_of_free_connections;
#endif
				connection->sockfd = connectionfd;
				ev.data.ptr = connection;
				ev.events = EPOLLIN | EPOLLET;
				res = epoll_ctl(server->epfd,EPOLL_CTL_ADD,
						connectionfd,&ev);
				if(res == -1){
					stp_release_connection(connection,server);
					printf("epoll_ctl error!\n");
					return 0;
				}
#ifdef DEBUG
				printf("new connection coming %d\n",getpid());
#endif
			}
		}
	}
#ifndef MASTER_MODEL
	res = epoll_ctl(server->epfd,EPOLL_CTL_DEL,
			server->listenfd,&ev);
	if(-1 == res)
		printf("del listenfd error\n");
#ifdef DEBUG
	else
		printf("del listenfd done\n");
#endif

#endif
	return 0;
}

void stp_epoll_wait_circle(stp_server_t *server){
	struct epoll_event events[EPOLL_MAX_EVENTS_NUM],del_ev;
	int epoll_n,i,currfd,res;
#ifndef MASTER_MODEL
	int channelfd = server->channel_socks[server->process_index][1];
#endif
	stp_event_t* ev;
	stp_list_t* event_node;
	stp_connection_t *connection;
#ifdef MASTER_MODEL
	//printf("epoll_wait()\n");
#endif
	epoll_n = epoll_wait(server->epfd,events,EPOLL_MAX_EVENTS_NUM,0);
	for(i = 0;i<epoll_n;++i){
		currfd = events[i].data.fd;
		if(currfd == server->listenfd){
#ifndef MASTER_MODEL
			if(server->is_hold_accpet_mutx)
				continue;
			else
#endif
				stp_accept_handler(server);
		}
#ifndef MASTER_MODEL
		else if(currfd == channelfd){
			if(!stp_channel_handler(server)){
				printf("channel_handler error !\n");
				exit(1);
			}
		}
#endif	
		else{
			connection = events[i].data.ptr;
#ifndef MASTER_MODEL
			if(server->is_hold_accpet_mutx){
				ev = stp_get_event(server);
				if(NULL == ev)
					continue;
				ev->connection = connection;
				ev->events = events[i].events;
				//stp_add_list_at_front(&server->post_used_event_list,&ev->list);
			}else{
#endif
				res = stp_connection_handler(
						connection,events[i].events);
				res = stp_epoll_operation_for_connection_handler_res(
						server,res,connection);
#ifndef MASTER_MODEL
			}
#endif
		}
	}
}

void stp_epoll_circle(stp_server_t *server){
	int try_res,disable_to_accept;
	while(1){
#ifndef MASTER_MODEL
		if(server->disable_to_accept > 0)
			--(server->disable_to_accept);
		else{
			try_res = stp_trylock_accept_mtx(server);
			/*
			if(!try_res && server->num_of_free_connections == 
					server->total_num_of_connections)
				continue;
				*/
			stp_epoll_wait_circle(server);
			if(try_res){
				stp_accept_handler(server);
				stp_shmtx_unlock(&server->accpet_mutx);
				server->is_hold_accpet_mutx = 0;
				if(stp_list_empty(&server->post_used_event_list))
					continue;
				stp_post_connection_handler(server);
			}
		}
#else
		stp_epoll_wait_circle(server);
#endif
	}
}

int setNonblocking(int fd){
	int flags;
	if(-1 == (flags = fcntl(fd,F_GETFL,0)))
		flags = 0;
	flags = fcntl(fd,F_SETFL,flags|O_NONBLOCK);
	return flags == 0;
}

stp_connection_t* stp_get_connection(stp_server_t *server){
	stp_list_t *connection_node;
	stp_connection_t *connection;
	if(stp_list_empty(&server->free_connections)){
		return NULL;
	}
	connection_node = stp_remove_listnode_at_front(
			&server->free_connections);
	connection = cast_to_object(stp_connection_t,
			list,connection_node);
	if(!stp_connection_init(connection)){
#ifdef MASTER_MODEL
		printf("stp_connection_init error\n");
#endif
		stp_add_list_at_tail(&server->free_connections,connection_node);
		return NULL;
	}
	stp_add_list_at_front(&server->used_connections,
			&connection->list);
	--(server->num_of_free_connections);
	return connection;
}

#ifndef MASTER_MODEL

int stp_channel_handler_core(int cmd,stp_server_t *server){
	switch(cmd){
		case STP_CMD_QUITE :
			stp_destroy_server(server,1);
			printf("cmd recved quit \n");
			exit(1);
			return 1;
		case STP_CMD_TERMINATE:
			printf("cmd recved terminate \n");
			exit(1);
			return 1;
		case STP_CMD_ERROR:
			return 1;
		default:
			return 0;
	}
}

int stp_channel_handler(stp_server_t *server){
	int count = 0;
	int cmd;
	int channelfd = server->channel_socks[server->process_index][1];
	server->channel_read_index = 0;
	while(1){
		count = stp_socket_read(1,
				server->channel_socks[server->process_index],
				server->channel_buff+server->channel_read_index,
				CHANNEL_BUFF_SIZE-server->channel_read_index-1);
		//printf("count = %d\n",count);
		if(count == -1){
			if(EINTR != errno && EWOULDBLOCK != errno && EAGAIN != errno){
				stp_channel_read_error_handler(server,channelfd);
				return 0;
			}else
				break;
		}else if(count == 0){
			return 0;
			//channel_close_handler
		}else{
			server->channel_read_index += count;
		}
	}
	if(server->channel_read_index < CHANNEL_BUFF_SIZE-1){
		server->channel_buff[server->channel_read_index] = 0;
		cmd = stp_socket_cmd_resolve(server->channel_buff);
		if(!stp_channel_handler_core(cmd,server))
			return 0;
		return 1;
	}else{
		printf("child recved channel error !\n");
		//channel_read_error_handler
		return 0;
	}
}

/*try to lock accept mtx and if lock add listenfd to epoll*/
int stp_trylock_accept_mtx(stp_server_t *server){
	int n;
	struct epoll_event ev;
	if(stp_shmtx_trylock(&server->accpet_mutx)){
#ifdef DEBUG
		printf("accept locked\n");
#endif
		ev.data.fd = server->listenfd;
		ev.events = EPOLLIN|EPOLLET;
		n = epoll_ctl(server->epfd,EPOLL_CTL_ADD,
				server->listenfd,&ev);
		if(-1 == n){
			stp_shmtx_unlock(&server->accpet_mutx);
			return 0;
		} 
		server->is_hold_accpet_mutx = 1;
		return 1;
	}
	if(server->is_hold_accpet_mutx){
		server->is_hold_accpet_mutx = 0;
		n = epoll_ctl(server->epfd,EPOLL_CTL_DEL,
				server->listenfd,&ev);
		if(-1 == n)
			return 0;
	}
	return 0;
}

void stp_channel_read_error_handler(stp_server_t *server,int channelfd){
	printf("%d channel read error, process exit\n",getpid());
	exit(-1);
}

int stp_post_connection_handler(stp_server_t *server){
	stp_list_t *used_event_list = &server->post_used_event_list;
	stp_list_t *free_event_list = &server->post_free_event_list;
	stp_list_t *curr;
	stp_event_t *event;
	int res = 1,tempres;
	while(!stp_list_empty(used_event_list)){
		curr = stp_remove_listnode_at_front(used_event_list);
		event = cast_to_object(stp_event_t,list,curr);
		tempres = stp_connection_handler(
				event->connection,event->events);
		tempres = stp_epoll_operation_for_connection_handler_res(server,
				tempres,event->connection);
		if(0 == tempres)
			res = 0;
		stp_add_list_at_front(free_event_list,curr);
	}
	return res;
}

stp_event_t* stp_get_event(stp_server_t *server){
	stp_event_t* ev;
	stp_list_t* event_node;
	if(stp_list_empty(&server->post_free_event_list))
		return NULL;
	event_node = stp_remove_listnode_at_front(
			&server->post_free_event_list);
	stp_add_list_at_front(&server->post_used_event_list,
			event_node);
	ev = cast_to_object(stp_event_t,list,event_node);
	return ev;
}
#endif

void stp_release_connection(stp_connection_t *connection,
		stp_server_t *server){
	stp_list_t *node = &connection->list;
	stp_list_t *list = &server->free_connections;
	stp_remove_list_node(node);
	stp_add_list_at_front(list,node);
	++(server->num_of_free_connections);
	stp_connection_reset(connection);
}

/**
 * do operations as the res code of connection_handler
 * if res == 0 means the socket is error so delete the epoll fd
 * if res == 1 when status is READ_REQUEST_HEAD means the socket
 * is still need to read head
 * if res == 1 when status is READ_HEAD_DONE means the socket
 * needs to be mod to EPOLLOUT event
 * if res == 1 when status is SEND_RESPONSE_HEAD means the socket
 * is still need to send head
 * if res == 1 when status is SEND_HEAD_DONE means the socket
 * is start to send response body
 * if res == 1 when status is SEND_RESPONSE means the socket
 * is still need to send response body
 * if res == 1 when status is SEND_DONE means the socket finish
 * the request the epoll fd can be del if the http version is 1.0
 * */
int stp_epoll_operation_for_connection_handler_res(
		stp_server_t *server,int res,stp_connection_t *connection){
	int status;
	struct epoll_event ev;
	if(0 == res)
		return stp_cleanup_connection(server,connection);
	status = connection->request.status;
	if(status == READ_HEAD_DONE){
		ev.data.ptr = connection;
		ev.events = EPOLLOUT|EPOLLET;
		res = epoll_ctl(server->epfd,EPOLL_CTL_MOD,
				connection->sockfd,&ev);
		if(-1 == res){
			printf("epoll mod connection sock error\n");
			stp_release_connection(connection,server);
			return 0;
		}
		return 1;
	}
	if(status == SEND_HEAD_DONE){
		if(connection->request.response_code != RESPONSE_CODE_200)
			return stp_cleanup_connection(server,connection);
	}
	if(status == SEND_DONE)
		return stp_cleanup_connection(server,connection);
	return 0;
}


int stp_cleanup_connection(stp_server_t *server,
		stp_connection_t *connection){
	int res;
	struct epoll_event ev;
	res = epoll_ctl(server->epfd,EPOLL_CTL_DEL,
				connection->sockfd,&ev);
	close(connection->sockfd);
	if(-1 == res){
		printf("epoll del connection sock error\n");
		stp_release_connection(connection,server);
		return 0;
	}
	stp_release_connection(connection,server);
#ifdef DEBUG
	sleep(10);
#endif
	return 1;
}
