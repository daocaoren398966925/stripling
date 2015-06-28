#include"stp_connection.h"
#include"stp_config.h"
#include<sys/epoll.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>

void stp_request_reset(stp_connection_t *connection){
	connection->request.read_index = 0;
	connection->request.write_index = 0;
	connection->request.total_length_to_write = 0;
	connection->request.file_len = 0;
	connection->request.status = READ_REQUEST_HEAD;
}

void stp_connection_reset(stp_connection_t *connection){
	connection->sockfd = 0;
	if(connection->pool != NULL){
		stp_destroy_pool(connection->pool);
	}
	connection->pool = NULL;
	connection->request.buff = NULL;
	stp_request_reset(connection);
	//stp_list_init(&connection->list);
}

int stp_connection_init(stp_connection_t *connection){
	stp_connection_reset(connection);
	connection->pool = stp_creat_pool(1024*4);
	connection->request.buff = stp_pool_alloc(connection->pool,
			REQUEST_BUFF_SIZE);
	if(connection->pool != NULL)
		return 1;
	return 0;
}

int stp_read_sock_loop(stp_connection_t *connection){
	int n = 0,sum = 0;
	stp_request_t* request = &connection->request;
	while(1){
		n = recv(connection->sockfd,request->buff+request->read_index,
				REQUEST_BUFF_SIZE-request->read_index,0);
		if(n == -1){
			if(EINTR == errno || EWOULDBLOCK == errno || 
						EAGAIN == errno){
#ifdef MASTER_MODEL
				request->buff[request->read_index] = 0;
#endif
				break;
			}else{
				stp_request_reset(connection);
				printf("connection read error\n");
			}
		}else if(n == 0){
			return 0;
		}else{
			request->read_index += n;
			sum += n;
		}
	}
	return sum;
}

int stp_connection_handler(stp_connection_t *connection,
		unsigned int events){
	int num = 0;
	char* is_complete;
	int res;
	int status;
	if(events & EPOLLERR){
		printf("epoll EPOLLERR\n");
		close(connection->sockfd);
		return 0;
	}
	if(events & EPOLLIN){
		num = stp_read_sock_loop(connection);
#ifdef MASTER_MODEL
		//printf("recv :%s\n",connection->request.buff);
#endif
		if(num == 0){
			return 0;
		}
		is_complete = strstr(connection->request.buff,"\n\n");
		if(NULL == is_complete)
			is_complete = strstr(connection->request.buff,"\r\n\r\n");
		if(NULL == is_complete)
			return 1;
		connection->request.buff[connection->request.read_index] = 0;
#ifdef MASTER_MODEL
		printf("recv %s\n",connection->request.buff);
#endif
		connection->request.status = READ_HEAD_DONE;
		stp_request_head_handler(connection);
		return 1;
	}
	if(events & EPOLLOUT){
		status = connection->request.status;
		res = 1;
		switch(status){
			case READ_HEAD_DONE:
				connection->request.status = SEND_RESPONSE_HEAD;
				res = stp_send_response_head(connection);
				break;
			case SEND_RESPONSE_HEAD:
				res = stp_send_response_head(connection);
				break;
			case SEND_HEAD_DONE:
				connection->request.status = SEND_RESPONSE;
				res = stp_send_response(connection);
				break;
			case SEND_RESPONSE:
				res = stp_send_response(connection);
				break;
			case SEND_DONE:
				break;
			default:
				break;
		}
 		if(0 == res)
			return 0;
		return 1;
	}
}

int stp_request_head_handler(stp_connection_t *connection){
	int res;
	char *end_line,*space_line;
	char *buff = connection->request.buff;
	char location[LOCATION_BUFF_SIZE];
	char *one_head_line;
	int path_len;
	int file_len;
	struct stat st;
	res = strncmp(buff,"GET",3);
	if(0 != res){
		stp_copy_msg_to_buff(connection,RESPONSE_CODE_400);
		return ;
	}
	end_line = strchr(buff,'\n');
	space_line = strchr(buff+4,' ');
	if(end_line < space_line){
		stp_copy_msg_to_buff(connection,RESPONSE_CODE_400);
		return ;
	}
	path_len = space_line-buff-4;
	if(strlen(WEB_ROOT)+path_len > LOCATION_BUFF_SIZE){
		stp_copy_msg_to_buff(connection,RESPONSE_CODE_400);
		return ;
	}
	strcpy(location,WEB_ROOT);
	strncpy(location+strlen(WEB_ROOT),buff+4,path_len);
	*(location+strlen(WEB_ROOT)+path_len) = 0;
	res = stat(location,&st);
	if(-1 == res){
		stp_copy_msg_to_buff(connection,RESPONSE_CODE_404);
		return ;
	}
	if(S_ISDIR(st.st_mode)){
		path_len = strlen(location)+strlen("index.html");
		if(path_len > LOCATION_BUFF_SIZE){
			stp_copy_msg_to_buff(connection,RESPONSE_CODE_400);
			return ;
		}
		strcpy(location+strlen(location),"index.html");
		res = stat(location,&st);
		if(-1 == res){
			stp_copy_msg_to_buff(connection,RESPONSE_CODE_404);
			return ;
		}
	}
	connection->request.file_fd = open(location,O_RDONLY);
	connection->request.file_len = st.st_size;
	if(-1 == connection->request.file_fd){
		stp_copy_msg_to_buff(connection,RESPONSE_CODE_403);
		return ;
	}
	stp_copy_msg_to_buff(connection,RESPONSE_CODE_200);
	file_len = st.st_size;
	one_head_line = location;
	snprintf(one_head_line,LOCATION_BUFF_SIZE,
			"Content-Length: %d\r\n",file_len);
	strcpy(buff+connection->request.total_length_to_write,one_head_line);
	connection->request.total_length_to_write += strlen(one_head_line);
	strcpy(buff+connection->request.total_length_to_write,"\r\n");
	connection->request.total_length_to_write += strlen("\r\n");
	return ;
}

/*call send(),return what send num of bytes return*/
int stp_send_response_head(stp_connection_t *connection){
	int res;
	stp_request_t *request = &connection->request;
	if(connection->request.write_index == 
			connection->request.total_length_to_write){
		connection->request.status = SEND_HEAD_DONE;
		return 1;
	}
	res = send(connection->sockfd,request->buff+request->write_index,
			request->total_length_to_write-request->write_index,0);
	if(0 == res)
		return 0;
	request->write_index += res;
	if(request->write_index == request->total_length_to_write){
		request->status = SEND_HEAD_DONE;
		if(request->response_code != RESPONSE_CODE_200)
			return 1;
		request->status = SEND_RESPONSE;
		request->write_index = 0;
		res = stp_send_response(connection);
		if(0 == res)
			return 0;
		return 1;
	}
	return 1;
}

int stp_send_response(stp_connection_t *connection){
	stp_request_t *request = &connection->request;
	off_t offset;
	int res;
	while(1){
		offset = request->write_index;
		res = sendfile(connection->sockfd,request->file_fd,&offset,
				request->file_len - request->write_index);
		request->write_index = offset;
		if(res == -1){
			if(errno == EAGAIN)
				return 1;
			else{
#ifdef MASTER_MODEL
				printf("sendfile error\n");
#endif
				return 0;
			}
		}
		if(request->write_index == request->file_len){
			request->status = SEND_DONE;
			close(connection->request.file_fd);
			return 1;
		}
	}
	return 1;
}

void stp_copy_msg_to_buff(stp_connection_t *connection,int res_code){
	char *buff = connection->request.buff;
	connection->request.response_code = res_code;
	switch(res_code){
		case RESPONSE_CODE_400:
			strcpy(buff,ERROR_MSG_400);
			connection->request.write_index = 0;
			connection->request.total_length_to_write = 
				strlen(ERROR_MSG_400);
			break;
		case RESPONSE_CODE_403:
			strcpy(buff,ERROR_MSG_403);
			connection->request.write_index = 0;
			connection->request.total_length_to_write = 
				strlen(ERROR_MSG_403);
			break;
		case RESPONSE_CODE_404:
			connection->request.write_index = 0;
			connection->request.total_length_to_write =
				strlen(ERROR_MSG_404);
			strcpy(buff,ERROR_MSG_404);
			break;
		case RESPONSE_CODE_200:
			connection->request.write_index = 0;
			connection->request.total_length_to_write =
				strlen(OK_MSG_200);
			strcpy(buff,OK_MSG_200);
			break;
		default:
			break;
	}
}



