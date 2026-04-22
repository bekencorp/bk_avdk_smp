/*
 * FreeRTOS+POSIX pthread / rwlock types for __Userspace__ + USRSCTP_PTHREAD_RWLOCK_COMPAT.
 *
 * Include this *before* <sys/types.h> (or any header that pulls newlib sys/types.h)
 * in .c/.h files that are compiled in that configuration. That prevents newlib stub
 * pthread_* from loading ahead of FreeRTOS_POSIX (see debug: conflicting pthread types).
 *
 * Safe to include multiple times; sctp_os_userspace.h also includes it before <sys/socket.h>
 * when the TU has not already included this file.
 */
#ifndef SCTP_OS_USERLAND_PTHREAD_H
#define SCTP_OS_USERLAND_PTHREAD_H

#if defined(USRSCTP_PTHREAD_RWLOCK_COMPAT)
#ifndef _SYS__PTHREADTYPES_H_
#define _SYS__PTHREADTYPES_H_
#endif
#include "pthread_rwlock_compat.h"
#elif defined(__Userspace__) && !defined(_WIN32) && !defined(__native_client__)
#include <pthread.h>
#endif

#if defined(SCTP_USE_LWIP)
#define IPVERSION  4
#define CMSG_ALIGN(len) (((len) + sizeof (size_t) - 1) \
			& (size_t) ~(sizeof (size_t) - 1))
#endif

typedef pthread_mutex_t userland_mutex_t;
typedef pthread_rwlock_t userland_rwlock_t;
typedef pthread_cond_t userland_cond_t;
typedef pthread_t userland_thread_t;

#endif /* SCTP_OS_USERLAND_PTHREAD_H */
