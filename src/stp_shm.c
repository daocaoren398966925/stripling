#include"stp_shm.h"
#include<sys/mman.h>
#include<stdio.h>

int stp_shm_alloc(stp_shm_t *shm){
	shm->addr = mmap(NULL,shm->size,PROT_READ|PROT_WRITE,
			MAP_ANON|MAP_SHARED,-1,0);
	if(shm->addr == MAP_FAILED)
		return 0;
	return 1;
}

int stp_shm_free(stp_shm_t *shm){
	if(munmap((void*)shm->addr,shm->size) == -1)
		return 0;
	return 1;
}
