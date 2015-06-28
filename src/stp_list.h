#ifndef STP_LIST_H
#define STP_LIST_H

typedef struct stp_list stp_list_t;

struct stp_list{
	stp_list_t* prev;
	stp_list_t* next;
};

#define offset(object,list) ((size_t)&(((object*)0)->list))
#define cast_to_object(object_t,list,currp) (object_t*)(((char*)currp)-offset(object_t,list))

inline int stp_list_empty(stp_list_t* list);

inline void stp_list_init(stp_list_t* list);

inline void stp_add_list_at_front(stp_list_t* list,stp_list_t* node);

inline void stp_add_list_at_tail(stp_list_t* list,stp_list_t* node);

inline void stp_remove_list_node(stp_list_t* node);

inline stp_list_t* stp_remove_listnode_at_front(stp_list_t* list);

inline void stp_add_list_node_before(stp_list_t* curr,stp_list_t* node);

#endif
