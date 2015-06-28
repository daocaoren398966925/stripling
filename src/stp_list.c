#include"stp_list.h"
#include<stdio.h>

int stp_list_empty(stp_list_t *list){
	return list->prev == list && list->next == list;
}

void stp_list_init(stp_list_t* list){
	list->next = list;
	list->prev = list;
}

void stp_add_list_at_front(stp_list_t* list,stp_list_t* node){
	node->next = list->next;
	list->next = node;
	node->next->prev = node;
	node->prev = list;
}

void stp_add_list_at_tail(stp_list_t* list,stp_list_t* node){
	list->prev->next = node;
	node->prev = list->prev;
	list->prev = node;
	node->next = list;
}

void stp_add_list_node_before(stp_list_t* curr,stp_list_t* node){
	node->next = curr;
	node->prev = curr->prev;
	curr->prev = node;
	node->prev->next = node;
}

void stp_remove_list_node(stp_list_t* node){
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->next = NULL;
	node->prev = NULL;
}

stp_list_t* stp_remove_listnode_at_front(stp_list_t *list){
	stp_list_t *node = list->next;
	stp_remove_list_node(node);
	return node;
}


