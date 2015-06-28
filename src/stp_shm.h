#ifndef STP_SHM_H
#define STP_SHM_H


typedef struct{
	char *addr;
	int size;
}stp_shm_t;


int stp_shm_alloc(stp_shm_t *shm);

int stp_shm_free(stp_shm_t *shm);


#endif
