#ifndef STP_CONFIG_H
#define STP_CONFIG_H

#define bool int
#define true 1
#define false 0

#define SERVER_PORT 8000

#define CHANNEL_BUFF_SIZE 100

#define WEB_ROOT "/home/yang/stripling/web_root"

#define DATA_PATH "/home/yang/stripling/data"

#define MAX_CONNECTION_NUM 5000

#define REQUEST_BUFF_SIZE 4096

#define LOCATION_BUFF_SIZE 512

#define CPU_NUM 2

#define CPU_BOUND 1

#define WORKPROCESS_NUM 2

#define EPOLL_MAX_EVENTS_NUM 1024

#define LISTEN_BLOCK_LENGTH 512

#endif
