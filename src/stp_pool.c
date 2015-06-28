#include"stp_pool.h"
#include<malloc.h>
#include<stdio.h>

stp_pool_t* stp_creat_pool(int size){
	stp_pool_t* pool;
	char* mem;
	pool = (stp_pool_t*)malloc(size);
	mem = (char*)pool;
	if(NULL == pool)
		return NULL;
	pool = (stp_pool_t*)mem;
	pool->last = mem+sizeof(stp_pool_t);
	pool->end = mem+size;
	pool->next = NULL;
	pool->current = pool;
	pool->failed_times = 0;
	pool->max = size-sizeof(stp_pool_t);
	pool->large = NULL;
	return pool;
}

void* stp_pool_alloc(stp_pool_t* pool,int size){
	stp_pool_t* curr_pool;
	char* res_mem;
	stp_large_mem_t* large,*temp;
	if(NULL == pool)
		return NULL;
	if(pool->max < size){
		large = malloc(sizeof(stp_large_mem_t));
		res_mem = (char*)malloc(size);
		large->data = res_mem;
		temp = pool->large;
		pool->large = large;
		large->next = temp;
		return res_mem;
	}
	curr_pool = pool->current;
	while(curr_pool){
		if(curr_pool->end - curr_pool->last > size){
			res_mem = curr_pool->last;
			curr_pool->last += size;
			return res_mem;
		}
		curr_pool->failed_times++;
		curr_pool = curr_pool->next;
	}
	return stp_add_new_pool_at_tail(pool,size);
}

void stp_destroy_pool(stp_pool_t* pool){
	stp_large_mem_t* large,*temp;
	stp_pool_t* ppool,*ptemp;
	if(NULL == pool)
		return;
	large = pool->large;
	while(large){
		temp = large;
		large = large->next;
		free(temp->data);
		free(temp);
	}
	pool->large = NULL;
	ppool = pool;
	while(ppool){
		ptemp = ppool;
		ppool = ppool->next;
		ptemp->next = NULL;
		free(ptemp);
	}
}

void* stp_add_new_pool_at_tail(stp_pool_t* pool,int size){
	stp_pool_t* pcurr;
	char* res,*newm;
	int psize = pool->end - (char*)pool;
	newm = (char*)malloc(psize);
	stp_pool_t* new_pool_node = (stp_pool_t*)newm;
	new_pool_node->end = newm+size;
	new_pool_node->failed_times = 0;
	new_pool_node->large = NULL;
	new_pool_node->last = newm+sizeof(stp_pool_t);
	new_pool_node->max = size-sizeof(stp_pool_t);
	new_pool_node->next = NULL;
	pcurr = pool->current;
	while(pcurr->next){
		if(pcurr->failed_times < 4)
			pool->current = pcurr;
		pcurr = pcurr->next;
	}
	pcurr->next = new_pool_node;
	if(pcurr->failed_times >= 4)
		pool->current = new_pool_node;
	res = new_pool_node->last;
	new_pool_node->last += size;
	return res;
}




