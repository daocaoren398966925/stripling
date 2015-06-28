#ifndef STP_SERVER_H
#define STP_SERVER_H

#include<netinet/in.h>
#include"stp_pool.h"
#include"stp_connection.h"
#include"stp_config.h"
#include"stp_shmtx.h"

typedef struct{
	unsigned int events;
	stp_connection_t *connection;
	stp_list_t list;
}stp_event_t;


typedef struct{
	stp_pool_t* pool;

	int epfd;
	int listenfd;
	int port;
	int listen_block_length;
	struct sockaddr_in addr_in;

	stp_list_t free_connections;
	int num_of_free_connections;
	stp_list_t used_connections;
	int total_num_of_connections;

#ifndef MASTER_MODEL
	/*var for load balance*/
	int disable_to_accept;

	/*mutx for accept event*/
	stp_shmtx_t accpet_mutx;
	int is_hold_accpet_mutx;

	/*double linked list for load balance*/
	stp_list_t post_free_event_list;
	stp_list_t post_used_event_list;

	/*socks for master and worker process communication*/
	int channel_socks[WORKPROCESS_NUM][2];
	char channel_buff[CHANNEL_BUFF_SIZE];
	int channel_read_index;

	/*
	 * process_index = -1 for master process
	 * process_index >=0 for work process
	 */
	int process_index;
	int work_process_num;
#endif

}stp_server_t;

typedef int (*stp_work_process_circle)(stp_server_t*);

bool stp_server_init(stp_server_t*);

void stp_destroy_server(stp_server_t*,int is_worker);

bool stp_start_listening(stp_server_t*);

int stp_start_work_process(int,stp_server_t*,stp_work_process_circle);

int stp_work_process(stp_server_t*);

/*creat epoll and ctl channel fd*/
static int stp_creat_epoll_and_init(stp_server_t*);

static void stp_epoll_circle(stp_server_t*);

static void stp_epoll_wait_circle(stp_server_t*);

static inline int setNonblocking(int fd);

static int stp_accept_handler(stp_server_t *server);

static stp_connection_t* stp_get_connection(stp_server_t*);

static inline void stp_release_connection(stp_connection_t*,stp_server_t*);

static inline int stp_epoll_operation_for_connection_handler_res(
		stp_server_t *,int res,stp_connection_t *);

static inline int stp_cleanup_connection(stp_server_t*,stp_connection_t*);

#ifndef MASTER_MODEL
static int stp_channel_handler_core(int,stp_server_t*);

static int stp_channel_handler(stp_server_t*);

static inline void stp_channel_read_error_handler(stp_server_t*,int);

static int stp_trylock_accept_mtx(stp_server_t*);

static stp_event_t* stp_get_event(stp_server_t *server);

static int stp_post_connection_handler(stp_server_t *);
#endif

#endif
