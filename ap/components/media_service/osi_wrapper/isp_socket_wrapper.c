#include "isp_socket_wrapper.h"

#ifdef CONFIG_LWIP_V2_1

#include <sockets.h>

static int isp_socket_wrapper(int domain, int type, int protocol)
{
    return socket(domain, type, protocol);
}

static int isp_closesocket_wrapper(int socket)
{
    return close(socket);
}

static int isp_bind_wrapper(int socket, const void *name, uint32_t namelen)
{
    return bind(socket, (const struct sockaddr *)name, (socklen_t)namelen);
}

static int isp_listen_wrapper(int socket, int backlog)
{
    return listen(socket, backlog);
}

static int isp_select_wrapper(int socket, int time_us)
{
    fd_set fdset;
    struct timeval tv;

    FD_ZERO(&fdset);
    FD_SET(socket, &fdset);
    tv.tv_sec = 0;
    tv.tv_usec = time_us;

    return select(socket + 1, &fdset, NULL, NULL, &tv);
}

static int isp_accept_wrapper(int s, void *addr, uint32_t *addrlen)
{
    socklen_t socklen = (socklen_t)*addrlen;
    int ret = accept(s, (struct sockaddr *)addr, &socklen);
    *addrlen = (uint32_t)socklen;
    return ret;
}

static int isp_connect_wrapper(int s, const void *addr, uint32_t addrlen)
{
    return connect(s, (const struct sockaddr *)addr, (socklen_t)addrlen);
}

static int isp_recv_wrapper(int socket, void *buf, size_t len, int flags)
{
    return recv(socket, buf, len, flags);
}

static int isp_send_wrapper(int socket, const void *buf, size_t len, int flags)
{
    return send(socket, buf, len, flags);
}

static int isp_recvfrom_wrapper(int socket, void *buf, size_t len, int flags, void *src_addr, uint32_t *addrlen)
{
    socklen_t socklen = (socklen_t)*addrlen;
    int ret = recvfrom(socket, buf, len, flags, (struct sockaddr *)src_addr, &socklen);
    *addrlen = (uint32_t)socklen;
    return ret;
}

static int isp_sendto_wrapper(int socket, const void *buf, size_t len, int flags, const void *dest_addr, uint32_t addrlen)
{
    return sendto(socket, buf, len, flags, (const struct sockaddr *)dest_addr, (socklen_t)addrlen);
}

static int isp_setsockopt_wrapper(int socket, int level, int optname, const void *optval, uint32_t optlen)
{
    return setsockopt(socket, level, optname, optval, (socklen_t)optlen);
}

static uint16_t isp_htons_wrapper(uint16_t hostshort)
{
    return htons(hostshort);
}

static uint16_t isp_ntohs_wrapper(uint16_t netshort)
{
    return ntohs(netshort);
}

static int isp_inet_pton_wrapper(int af, const char *src, void *dst)
{
    return inet_pton(af, src, dst);
}

static const char *isp_inet_ntop_wrapper(int af, const void *src, char *dst, uint32_t size)
{
    return inet_ntop(af, src, dst, size);
}

static bk_isp_socket_funcs_t s_isp_socket_funcs =
{
    .socket      = isp_socket_wrapper,
    .closesocket = isp_closesocket_wrapper,
    .bind        = isp_bind_wrapper,
    .listen      = isp_listen_wrapper,
    .select      = isp_select_wrapper,
    .accept      = isp_accept_wrapper,
    .connect     = isp_connect_wrapper,
    .recv        = isp_recv_wrapper,
    .send        = isp_send_wrapper,
    .recvfrom    = isp_recvfrom_wrapper,
    .sendto      = isp_sendto_wrapper,
    .setsockopt  = isp_setsockopt_wrapper,
    .htons       = isp_htons_wrapper,
    .ntohs       = isp_ntohs_wrapper,
    .inet_pton   = isp_inet_pton_wrapper,
    .inet_ntop   = isp_inet_ntop_wrapper,
};

extern int vsios_socket_adapter_init(void *funcs);

bk_err_t bk_isp_socket_funcs_init(void)
{
    bk_err_t ret = BK_OK;
    if (vsios_socket_adapter_init(&s_isp_socket_funcs) != 0)
    {
        ret = BK_FAIL;
    }
    return ret;
}

#endif // CONFIG_LWIP_V2_1