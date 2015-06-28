#ifndef STP_CONNECTION_H
#define STP_CONNECTION_H

#include"stp_pool.h"
#include"stp_list.h"

#define READ_SEND_ZERO      0
#define READ_REQUEST_HEAD   1
#define READ_HEAD_DONE      2
#define SEND_RESPONSE_HEAD  3
#define SEND_HEAD_DONE      4
#define SEND_RESPONSE		5
#define SEND_DONE		    6

#define RESPONSE_CODE_200	0	
#define RESPONSE_CODE_400	1
#define RESPONSE_CODE_404	2
#define RESPONSE_CODE_403	3

#define ERROR_MSG_400 "HTTP/1.0 400 Bad request\r\n" \
	"Server: stripling/1.0\r\n" \
	"Content-Type: text/html\r\n" \
	"Connection: Close\r\n\r\n<h1>Bad request</h1>"

#define ERROR_MSG_404 "HTTP/1.0 404 Not Found\r\n" \
	"Server: stripling/1.0\r\n" \
	"Content-Type: text/html\r\n" \
	"Connection: Close\r\n\r\n<h1>Not found</h1>"

#define ERROR_MSG_403 "HTTP/1.0 403 Forbidden\r\n" \
	"Server: stripling/1.0\r\n" \
	"Content-Type: text/html\r\n" \
	"Connection: Close\r\n\r\n<h1>Forbidden</h1>"

#define OK_MSG_200 "HTTP/1.0 200 OK\r\n" \
	"Server: stripling/1.0\r\n" \
	"Content-Type: text/html\r\n" \
	"Connection: Close\r\n"


typedef struct{
	char *buff;
	int total_length_to_write;
	int status;
	int response_code;
	int read_index;
	int write_index;
	int file_fd;
	int file_len;
}stp_request_t;

typedef struct{
	int sockfd;
	stp_pool_t* pool;
	stp_request_t request;
	stp_list_t list;
}stp_connection_t;

static inline void stp_request_reset(stp_connection_t *);

/*reset request read and write index, destroy connection pool*/
void stp_connection_reset(stp_connection_t *connection);

/*reset connection and create connection pool*/
int stp_connection_init(stp_connection_t *connection);

int stp_connection_handler(stp_connection_t *,unsigned int );

static int stp_read_sock_loop(stp_connection_t *);

static int stp_request_head_handler(stp_connection_t *);

static int stp_send_response_head(stp_connection_t*);

static int stp_send_response(stp_connection_t*);

static void stp_copy_msg_to_buff(stp_connection_t*,int);

#endif
