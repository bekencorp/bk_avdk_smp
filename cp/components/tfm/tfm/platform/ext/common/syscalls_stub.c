/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * NOTE: When GNU Arm compiler version greater equal than *11.3.Rel1*, there is a linker issue that
 * some system calls are not implemented, such as _close, _fstat and _getpid etc. So add this file
 * including stub functions of system calls to avoid the above linker issue.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

__attribute__((weak))
void _close(void)
{
}

__attribute__((weak))
void _fstat(void)
{
}

__attribute__((weak))
void _getpid(void)
{
}

__attribute__((weak))
void _isatty(void)
{
}

__attribute__((weak))
void _kill(void)
{
}

__attribute__((weak))
void _lseek(void)
{
}

__attribute__((weak))
void _read(void)
{
}

__attribute__((weak))
void _write(void)
{
}

/* Newlib's nano _gettimeofday is a nosys stub that emits a link-time warning
 * ("_gettimeofday is not implemented and will always fail"); with the TF-M
 * -fatal-warnings link option this aborts the build. Provide a quiet stub.
 * The secure world has no wall-clock; callers only reach this via library
 * paths that tolerate failure. */
__attribute__((weak))
int _gettimeofday(void *tv, void *tz)
{
	(void)tv;
	(void)tz;
	return -1;
}

/* Heap boundaries provided by the linker script. */
extern char __HeapBase[];
extern char __HeapLimit[];

/* Bounded _sbrk: the libnosys implementation has no upper limit and would
 * silently overrun .heap into the PSA RoT data placed right after it. */
void *_sbrk(ptrdiff_t incr)
{
	static char *heap_end;
	char *prev;
	size_t size;

	if (heap_end == NULL) {
		heap_end = __HeapBase;
	}

	prev = heap_end;

	if (incr >= 0) {
		size = (size_t)incr;
		if (size > (size_t)(__HeapLimit - prev)) {
			errno = ENOMEM;
			return (void *)-1;
		}
		heap_end = prev + size;
	} else {
		/* Negate without overflowing on PTRDIFF_MIN. */
		size = (size_t)(-(incr + 1)) + 1U;
		if (size > (size_t)(prev - __HeapBase)) {
			errno = ENOMEM;
			return (void *)-1;
		}
		heap_end = prev - size;
	}

	return prev;
}
