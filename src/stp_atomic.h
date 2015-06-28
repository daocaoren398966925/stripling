#ifndef STP_ATOMIC_H
#define STP_ATOMIC_H

#include<stdio.h>

typedef volatile int stp_atomic_t;

/*
 * if (*lock == old){
 *    *lock = set;
 *    return 1;
 * }
 * return 0
 */
inline stp_atomic_t stp_atomic_cmp_set(stp_atomic_t* lock,
		stp_atomic_t old,stp_atomic_t set);
/*
 *  temp = *value;
 *  *value += add;
 *  return temp
 */
inline stp_atomic_t stp_atomic_fetch_add(stp_atomic_t* value,stp_atomic_t add);


#endif
