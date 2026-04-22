/**
 * Newlib syscall stubs for kvs_aws_sample (arm-none-eabi).
 * Implements _gettimeofday, _open, _stat, _unlink so that libc's
 * _gettimeofday_r, _open_r, _stat_r, _unlink_r resolve to real
 * Beken RTC and bk_posix (VFS) instead of the "not implemented, will always fail" stubs.
 */
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <driver/aon_rtc.h>
#include "bk_posix.h"

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
	(void)ptimezone;
	if (!ptimeval) {
		errno = EINVAL;
		return -1;
	}
	if (bk_rtc_gettimeofday(ptimeval, NULL) != 0) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int _open(const char *path, int flags, ...)
{
	va_list ap;
	va_start(ap, flags);
	(void)va_arg(ap, int); /* mode when O_CREAT */
	va_end(ap);
	return open(path, flags);
}

int _stat(const char *path, struct stat *st)
{
	if (!st) {
		errno = EINVAL;
		return -1;
	}
	return stat(path, st);
}

int _fstat(int fd, struct stat *st)
{
	return fstat(fd, st);
}

int _unlink(const char *path)
{
	return unlink(path);
}

int _write(int file, const char *ptr, int len)
{
	if (ptr == NULL || len < 0)
		return -1;
	if (file == 1 || file == 2) { /* stdout / stderr */
		int orig_len = len;
		char buf[128];
		int n;
		while (len > 0) {
			n = len > (int)sizeof(buf) - 1 ? (int)sizeof(buf) - 1 : len;
			for (int i = 0; i < n; i++)
				buf[i] = ptr[i];
			buf[n] = '\0';
			os_printf("%s", buf);
			ptr += n;
			len -= n;
		}
		return orig_len;
	}
	return (int)write(file, (const void *)ptr, (size_t)len);
}
