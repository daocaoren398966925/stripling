#include"stp_atomic.h"


stp_atomic_t stp_atomic_cmp_set(stp_atomic_t* lock,
		stp_atomic_t old, stp_atomic_t set){
	char res;
	__asm__ volatile (
			"lock;"
			"cmpxchgl %3, %1;"
			"sete %0;"
			:"=a"(res):"m"(*lock),"a"(old),"r"(set):"cc","memory"
			);
	return res;
}

stp_atomic_t stp_atomic_fetch_add(stp_atomic_t* value,
		stp_atomic_t add){
	__asm__ volatile (
			" lock; "
			" xaddl %2, %1; "
			:"=a" (add):"m" (*value),"a" (add):"cc","memory"
			);
	return add;
}

