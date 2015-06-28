#ifndef STP_CHANNEL_H
#define STP_CHANNEL_H

#define STP_CMD_ERROR 0
#define STP_CMD_QUITE 1
#define STP_CMD_TERMINATE 2

#define STP_CMD_QUITE_STR "1"
#define STP_CMD_TERMINATE_SER "2"

int stp_socket_cmd_resolve(char*);

int stp_init_socketpair(int socket[2]);

void stp_socket_close(int is_work_pro,int socket[2]);

int stp_socket_write(int is_work_pro,int socket[2],void*buf,int size);

int stp_socket_read(int is_work_pro,int socket[2],void*buf,int size);

#endif
