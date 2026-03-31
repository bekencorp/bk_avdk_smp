#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>  // for size_t
#include <os/os.h>

typedef struct
{
    int (*socket)(int domain, int type, int protocol);
    int (*closesocket)(int socket);
    int (*bind)(int socket, const void *name, uint32_t namelen);
    int (*listen)(int socket, int backlog);
    int (*select)(int socket, int time_us);
    int (*accept)(int socket, void *addr, uint32_t *addrlen);
    int (*connect)(int socket, const void *addr, uint32_t addrlen);
    int (*recv)(int socket, void *buf, size_t len, int flags);
    int (*send)(int socket, const void *buf, size_t len, int flags);
    int (*recvfrom)(int socket, void *buf, size_t len, int flags, void *from, uint32_t *fromlen);
    int (*sendto)(int socket, const void *buf, size_t len, int flags, const void *dest_addr, uint32_t addrlen);
    int (*setsockopt)(int socket, int level, int optname, const void *optval, uint32_t optlen);
    uint16_t (*htons)(uint16_t hostshort);
    uint16_t (*ntohs)(uint16_t netshort);
    int (*inet_pton)(int af, const char *src, void *dst);
    const char *(*inet_ntop)(int af, const void *src, char *dst, uint32_t size);
} bk_isp_socket_funcs_t;

bk_err_t bk_isp_socket_funcs_init(void);

#ifdef __cplusplus
}
#endif

