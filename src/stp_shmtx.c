#include"stp_shmtx.h"
#include"stp_config.h"
#include<sched.h>

int stp_shmtx_create(stp_shmtx_t *mtx){
	stp_atomic_t val;
	stp_shm_t shm;
	shm.size = sizeof(stp_atomic_t);
	if(stp_shm_alloc(&shm) == 0)
		return 0;
	mtx->lock = (stp_atomic_t*)shm.addr;
	val = *mtx->lock;
	if(!stp_atomic_cmp_set(mtx->lock,val,0x00000000))
		return 0;
	if(mtx->spin == -1)
		return 1;
	mtx->spin = 2048;
	return 1;
}

int stp_shmtx_destory(stp_shmtx_t *mtx){
	stp_shm_t shm;
	shm.size = sizeof(stp_atomic_t);
	shm.addr = (char*)mtx->lock;
	return stp_shm_free(&shm);
}

int stp_shmtx_lock(stp_shmtx_t *mtx){
	int i,spin;
	stp_atomic_t val;
	while(1){
		val = *mtx->lock;
		if(val & 0x80000000 == 0){
			if(stp_atomic_cmp_set(mtx->lock,val,val|0x80000000))
				return 1;
		}
		if(CPU_NUM > 1){
			for(spin = 0;spin<mtx->spin;++spin){
				for(i = 0;i<spin;++i)
					stp_cpu_pause();
				val = *mtx->lock;
				if(val & 0x80000000 == 0){
					if(stp_atomic_cmp_set(mtx->lock,val,val|0x80000000))
							return 1;
				}
			}
		}
		sched_yield();
	}
}

int stp_shmtx_trylock(stp_shmtx_t *mtx){
	stp_atomic_t val;
	val = *mtx->lock;
	if((val & 0x80000000) == 0){
		if(stp_atomic_cmp_set(mtx->lock,val,val|0x80000000))
			return 1;
	}
	return 0;
}

int stp_shmtx_unlock(stp_shmtx_t *mtx){
	stp_atomic_t val,old;
	while(1){
		old = *mtx->lock;
		if((old & 0x80000000) == 0)
			return 0;
		val = 0x00000000;
		if(stp_atomic_cmp_set(mtx->lock,old,val)){
#ifdef DEBUG
			printf("accept unlocked\n");
#endif
			return 1;
		}
	}
}
