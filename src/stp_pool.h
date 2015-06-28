#ifndef STP_POOL_H
#define STP_POOL_H


typedef struct stp_large_mem stp_large_mem_t;

struct stp_large_mem{
	void* data;
	stp_large_mem_t* next;
};

typedef struct stp_pool stp_pool_t;

struct stp_pool{
	char* last;
	char* end;
	stp_pool_t* next;
	stp_pool_t* current;
	stp_large_mem_t* large;
	int failed_times;
	int max;
};

stp_pool_t* stp_creat_pool(int size);

void* stp_pool_alloc(stp_pool_t* pool,int size);

void stp_destroy_pool(stp_pool_t* pool);

static void* stp_add_new_pool_at_tail(stp_pool_t* pool,int size);

#endif
