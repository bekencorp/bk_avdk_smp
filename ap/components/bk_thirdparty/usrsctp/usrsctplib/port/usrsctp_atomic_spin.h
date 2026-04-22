/*
 * Serialized int atomics for Beken Armino (same idea as kvs_aws AtomicsGnu.h:
 * rtos_disable_int + spin_lock around read-modify-write).
 * Implemented in usrsctp_atomic_spin.c so one global lock is shared across all TUs.
 */
#ifndef USRSCTP_ATOMIC_SPIN_H
#define USRSCTP_ATOMIC_SPIN_H

int usrsctp_atomic_fetch_add_int(volatile int *p, int v);
void usrsctp_atomic_add_int(volatile int *p, int v);
void usrsctp_atomic_subtract_int(volatile int *p, int v);
int usrsctp_atomic_cmpset_int(volatile int *dst, int exp, int src);

#endif /* USRSCTP_ATOMIC_SPIN_H */
