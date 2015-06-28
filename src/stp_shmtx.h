#ifndef STP_SHMTX_H
#define STP_SHMTX_H

#include"stp_atomic.h"
#include"stp_shm.h"

#define stp_cpu_pause() __asm__("pause")

typedef struct{
	stp_atomic_t *lock;
	int spin;
}stp_shmtx_t;


int stp_shmtx_create(stp_shmtx_t *mtx);

int stp_shmtx_destory(stp_shmtx_t *mtx);

int stp_shmtx_trylock(stp_shmtx_t *mtx);

int stp_shmtx_lock(stp_shmtx_t *mtx);

int stp_shmtx_unlock(stp_shmtx_t *mtx);

#endif
