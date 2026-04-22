/**
 * sys/uio.h compatibility for beken (no newlib sys/uio.h).
 * POSIX scatter-gather I/O: struct iovec for readv/writev.
 * When SCTP_USE_LWIP is defined, struct iovec is already provided by
 * lwip/sockets.h (included via sys/socket.h), so we must not redefine it.
 */
#ifndef SYS_UIO_H
#define SYS_UIO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(SCTP_USE_LWIP)
/* No lwIP: define struct iovec here (e.g. standalone build). */
struct iovec {
	void  *iov_base;
	size_t iov_len;
};
#endif
/* With SCTP_USE_LWIP: struct iovec comes from lwip/sockets.h (via sys/socket.h). */

#ifdef __cplusplus
}
#endif

#endif /* SYS_UIO_H */
