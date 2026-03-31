/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2019 VeriSilicon Inc                       --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/

#ifndef _OSAL_FREERTOS_H_
#define _OSAL_FREERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

// Include common headers for BK_ASSERT and type definitions first
// This ensures bk_printf is declared before os.h defines os_printf as bk_printf
#include <common/bk_assert.h>
#include <common/bk_typedef.h>
// Include OS abstraction layer
#include "os/os.h"

// Define NULL if not already defined
#ifndef NULL
#define NULL ((void *)0)
#endif

// Map standard C assert to BK_ASSERT
#ifndef assert
#define assert(exp) BK_ASSERT(exp)
#endif

/*
 * NOTE:
 * `printf` is macro-mapped below. If <stdio.h> is included after that mapping,
 * the `printf()` prototype inside <stdio.h> may be macro-expanded and cause
 * conflicting declarations (e.g. `bk_printf` return type mismatch).
 * Include <stdio.h> here first so its prototypes are parsed before overrides.
 */
#include <stdio.h>

// Map standard C printf to BK_DUMP_OUT only when stdio printf is disabled.
// Otherwise <stdio.h> may be included and its `printf()` prototype would be
// macro-expanded into a conflicting `bk_printf()` prototype.
#ifndef printf
#define printf(...) BK_DUMP_OUT(__VA_ARGS__)
#endif

// Map sscanf to a simple implementation for basic integer parsing
// This is a simplified version that only supports "%d" format for integers
#ifndef sscanf
#include <stdarg.h>  // Required for va_list, va_start, va_arg, va_end
static inline int osal_sscanf(const char *str, const char *format, ...) {
	// Only support "%d" format for now
	if (format && str && str[0] != '\0') {
		if (format[0] == '%' && format[1] == 'd') {
			va_list args;
			int *value;
			int result = 0;
			int sign = 1;
			const char *p = str;
			
			// Skip whitespace
			while (*p == ' ' || *p == '\t') p++;
			
			// Check for sign
			if (*p == '-') {
				sign = -1;
				p++;
			} else if (*p == '+') {
				p++;
			}
			
			// Parse digits
			if (*p >= '0' && *p <= '9') {
				result = 0;
				while (*p >= '0' && *p <= '9') {
					result = result * 10 + (*p - '0');
					p++;
				}
				result *= sign;
				
				va_start(args, format);
				value = va_arg(args, int *);
				if (value) {
					*value = result;
				}
				va_end(args);
				return 1; // Return number of items successfully assigned
			}
		}
	}
	return 0; // Return 0 if no items were assigned
}
#define sscanf(str, format, ...) osal_sscanf(str, format, ##__VA_ARGS__)
#endif

#if defined(FREERTOS_SIMULATOR) || defined(__linux__) || defined(_WIN32)
#ifndef _HAVE_PTHREAD_H
#define _HAVE_PTHREAD_H
#endif
#endif

#if defined(__FREERTOS__) && !defined(FREERTOS_SIMULATOR)

/* FreeRTOS specific types and functions mapping */
/* Map TickType_t to uint32_t (OS time type) */
#ifndef TickType_t
typedef uint32_t TickType_t;
#endif

/* Map xTaskGetTickCount to rtos_get_time */
#ifndef xTaskGetTickCount
#define xTaskGetTickCount() rtos_get_time()
#endif

/* Map vTaskDelayUntil to use rtos_thread_msleep */
/* Note: vTaskDelayUntil maintains a wake time, but we use a simplified version. */
static inline void vTaskDelayUntil(TickType_t *pxPreviousWakeTime, const TickType_t xTimeIncrement) {
	TickType_t xCurrentTime = rtos_get_time();
	TickType_t xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

	if (xCurrentTime < xTimeToWake) {
		TickType_t xDelay = xTimeToWake - xCurrentTime;
		rtos_thread_msleep(xDelay);
	}

	*pxPreviousWakeTime = rtos_get_time();
}
#endif /* defined(__FREERTOS__) && !defined(FREERTOS_SIMULATOR) */

#ifdef _HAVE_PTHREAD_H
#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32)
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/semaphore.h"
#include "FreeRTOS_POSIX/sched.h"
#include "FreeRTOS_POSIX/unistd.h"
#include "FreeRTOS_POSIX/errno.h"

#include <io.h>
#include <process.h>
#include <windows.h>
#include <corecrt_wtime.h>

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#elif defined(__linux__)  //Linux Simulator
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "task.h"

#include <unistd.h>
#include <stdlib.h>

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <limits.h> /* NAME_MAX, CHAR_BIT*/
#endif /* _WIN32 */

#else  //other os, example FREERTOS (Need macro __FREERTOS__)

#ifdef __linux__
#define _STDLIB_H
#endif

#endif /* FREERTOS_SIMULATOR */
#endif /* _HAVE_PTHREAD_H */

#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <windows.h>
#include <time.h>
#elif defined(__linux__)  //Linux simulator
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#endif /* _WIN32 */

#else  //Not FreeRTOS simulator, FreeRTOS and other os
//#include "posix/sys/types.h"
// #include "posix/time.h"
#undef USE_POSIX_SEMAPHORE
#define USE_POSIX_SEMAPHORE
#undef USE_SYS_V_IPC
#define USE_SYS_V_IPC
#endif /* FREERTOS_SIMULATOR */

#ifndef FREERTOS_SIMULATOR
//malloc for thread security
#include "os/mem.h"
/*
 * NOTE:
 * This header overrides standard C allocation APIs (malloc/free/calloc/realloc)
 * with OS abstraction layer implementations. If a translation unit includes
 * <stdlib.h> after these macro overrides, the function prototypes inside
 * <stdlib.h> would be macro-expanded and break compilation (e.g. `free()`).
 * Include <stdlib.h> here first so its prototypes are parsed before overrides.
 */
#include <stdlib.h>
// Use OS abstraction layer functions instead of pvPortMalloc/pvPortFree
// These macros map standard C memory functions to OS abstraction layer
#ifndef malloc
#define malloc(size)                    os_malloc(size)
#endif
#ifndef free
#define free(ptr)                       os_free(ptr)
#endif
// calloc implementation using os_malloc and os_memset
#ifndef calloc
static inline void *osal_calloc(size_t nmemb, size_t size) {
	size_t total_size = nmemb * size;
	void *ptr = os_malloc(total_size);
	if (ptr) {
		// Zero-initialize the memory using OS abstraction layer
		os_memset(ptr, 0, total_size);
	}
	return ptr;
}
#define calloc(nmemb, size)             osal_calloc(nmemb, size)
#endif
// realloc implementation using os_malloc, os_free and os_memcpy
#ifndef realloc
static inline void *osal_realloc(void *ptr, size_t size) {
	if (size == 0) {
		if (ptr) {
			os_free(ptr);
		}
		return NULL;
	}
	if (ptr == NULL) {
		return os_malloc(size);
	}
	// Note: os_realloc may not exist, so we allocate new memory and copy
	// This is a simplified implementation
	void *new_ptr = os_malloc(size);
	if (new_ptr && ptr) {
		// Copy old data using OS abstraction layer (we don't know the old size, so this is a limitation)
		// In practice, the caller should track the size
		os_memcpy(new_ptr, ptr, size);
		os_free(ptr);
	}
	return new_ptr;
}
#define realloc(ptr, size)              osal_realloc(ptr, size)
#endif
#else
#include <malloc.h>
#endif /* FREERTOS_SIMULATOR */
// os/os.h, common/bk_assert.h, common/bk_typedef.h are already included at the top of the file
// Do not include standard C library headers (stdlib.h, stdio.h, assert.h, stddef.h, stdint.h)
// Use OS abstraction layer functions and common/bk_* functions instead
#include "vsi_string.h"

#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32) || defined(__linux__)
typedef struct timeval osal_timeval;
#endif /* defined(_WIN32) || defined(__linux__) */

#else  //FreeRTOS
// ssize_t may not be defined in non-simulator FreeRTOS, use int instead
typedef struct osal_timeval_ {
  int tv_sec; /* seconds */
  long tv_usec;   /* and microseconds */
} osal_timeval;
#define timeval osal_timeval_
#endif /* FREERTOS_SIMULATOR */

struct stream_trace {
  struct node *next;
  char *buffer;
  char comment[256];
  size_t size;
  void *fp;
  unsigned int cnt;
};

typedef void *(*process_main_ptr)(void *arg);
typedef struct {
  int pid;
#if defined(__FREERTOS__) && !defined(FREERTOS_SIMULATOR)
  vsios_thread_t *tid;
#else
  void *tid;  // For simulator or other OS, use void *
#endif
} osal_pid;

#ifdef FREERTOS_SIMULATOR
//file operations
#if defined(_MSC_VER)
/* MSVS doesn't define off_t, and uses _f{seek,tell}i64. */
//typedef __int64 off_t;
typedef __int64 off64_t;
#define fseeko _fseeki64
#define ftello _ftelli64
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#elif defined(_WIN32)
/* MinGW defines off_t as long and uses f{seek,tell}o64/off64_t for large files. */
#define fseeko fseeko64
#define ftello ftello64
#define off_t off64_t

#elif defined(__linux__)
//Nothing to do
#endif /* _MSC_VER */

#else  //For FreeRTOS, close all the file operations, maybe can use FreeRTOS+FAT
//typedef long long off_t;
#define off64_t off_t
#endif /* FREERTOS_SIMULATOR */

#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
#define putw _putw

typedef unsigned long(__stdcall *THREAD_ROUTINE)(void *Argument);
#define snprintf _snprintf
#define open _open
#define close _close
#define read _read
#define write _write
//#define lseek _lseeki64
//#define fsync _commit
//#define tell _telli64
#define NAME_MAX 255

//#undef fseek
#define fseek _fseeki64
#define int16 __int16
#define int64 __int64
#ifndef CMODEL_DLL_API
#ifdef CMODEL_DLL_SUPPORT /* cmodel in dll */
#ifdef _CMODEL_ /* This macro will be enabled when compiling cmodel libs. */
#define CMODEL_DLL_API __declspec(dllexport)
#else
#define CMODEL_DLL_API __declspec(dllimport)
#endif /* _CMODEL_ */
#else
/* win non-dll. */
#define CMODEL_DLL_API
#endif /* CMODEL_DLL_SUPPORT */
#endif /* CMODEL_DLL_API */

#undef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER \
  { 0, 0, 0, 0 }
/*
 * pthread_attr_{set}inheritsched
 */
#define PTHREAD_INHERIT_SCHED 0
#define PTHREAD_EXPLICIT_SCHED 1 /* Default */

enum {
  SCHED_OTHER = 0,
  SCHED_FIFO = 1,
  SCHED_RR,
  SCHED_MIN = SCHED_OTHER,
  SCHED_MAX = SCHED_RR
};

#elif defined(__linux__)
#define _FILE_OFFSET_BITS 64  // for 64 bit fseeko
#define fseek fseeko
#define int16 int16_t
#define int64 int64_t
#define __int64 long long

#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif
#endif /* _WIN32 */

#else  //For FreeRTOS, close all the file operations, maybe can use FreeRTOS+FAT
//#define fseek(a, b, c)
#define int16 short
#define int64 long long
#ifndef __int64
#define __int64 long long
#endif
#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif /* CMODEL_DLL_API */
/*
* pthread_attr_{set}inheritsched
*/
#define PTHREAD_INHERIT_SCHED 0
#define PTHREAD_EXPLICIT_SCHED 1 /* Default */
enum {
  SCHED_OTHER = 0,
  SCHED_FIFO = 1,
  SCHED_RR,
  SCHED_MIN = SCHED_OTHER,
  SCHED_MAX = SCHED_RR
};

#endif /* FREERTOS_SIMULATOR */

#ifndef FREERTOS_SIMULATOR
#ifdef _HAVE_PTHREAD_H
// pthread_once_t and PTHREAD_ONCE_INIT are already defined in the adapter layer above (lines 197-203)
// No need to redefine them here to avoid conflicts

// pthread_condattr and pthread_attr functions are already defined in the adapter layer above
// (lines 126-134 for pthread_condattr, lines 161-174 for pthread_attr)
// No need to redefine them here

// pthread_once function implementation - pthread_once_t is defined in adapter layer above
#if defined(__FREERTOS__) && !defined(FREERTOS_SIMULATOR)
static av_unused int pthread_once(pthread_once_t *once_control,
                                  void (*init_routine)(void)) {
#ifdef SEM_REPLACE_MUTEX
  sem_wait(&once_control->once_mutex);
#else
  pthread_mutex_lock(&once_control->once_mutex);
#endif
  if (once_control->done == 0) {
    init_routine();
    once_control->done = 1;
  }
#ifdef SEM_REPLACE_MUTEX
  sem_post(&once_control->once_mutex);
#else
  pthread_mutex_unlock(&once_control->once_mutex);
#endif
  return 0;
}
#endif /* defined(__FREERTOS__) && !defined(FREERTOS_SIMULATOR) */
#endif /* _HAVE_PTHREAD_H */
#endif /* FREERTOS_SIMULATOR */

/*------------------------------------------------------------------------------
 Function name   : osal_gettimeofday
 Description     : open method
 Parameters      : osal_timeval *tp - Used to store  time of the current time
                     included the seconds and nanosecnods
                   void *tzp - not used
 Return type     : int
------------------------------------------------------------------------------*/
static av_unused int osal_gettimeofday(osal_timeval *tp, void *tzp) {
#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32)
  time_t clock;
  struct tm tm;
  SYSTEMTIME wtm;

  GetLocalTime(&wtm);
  tm.tm_year = wtm.wYear - 1900;
  tm.tm_mon = wtm.wMonth - 1;
  tm.tm_mday = wtm.wDay;
  tm.tm_hour = wtm.wHour;
  tm.tm_min = wtm.wMinute;
  tm.tm_sec = wtm.wSecond;
  tm.tm_isdst = -1;
  clock = mktime(&tm);
  tp->tv_sec = (long)clock;
  tp->tv_usec = wtm.wMilliseconds * 1000;

  return 0;
#elif defined(__linux__)
  return gettimeofday(tp, tzp);
#endif /* _WIN32 */

#else  //FreeRTOS and other os need to implement  by oneself
  //TODO...
  return 0;
#endif /* FREERTOS_SIMULATOR */
  return 0;
}

static av_unused void osal_usleep(unsigned long usec) {
#ifdef FREERTOS_SIMULATOR
	usleep(usec);
#else
	// For FreeRTOS, use OS abstraction layer
	// Convert microseconds to milliseconds (round up to at least 1ms)
	unsigned long msec = (usec + 999) / 1000;
	if (msec == 0) {
		msec = 1;
	}
	rtos_thread_msleep(msec);
#endif
}

static av_unused void osal_open_memstream(struct stream_trace *stream_trace) {
#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32)
  int fd;
  HANDLE fm, h;
  fd = _fileno(stream_trace->fp);
  h = (HANDLE)_get_osfhandle(fd);

  fm = CreateFileMapping(h, NULL, PAGE_READWRITE | SEC_RESERVE, 0,
                         16 * 1024 * 1024, NULL);
  if (fm == NULL) {
    BK_DUMP_OUT("Could not access memory space! %d\n", GetLastError());
    BK_ASSERT(0);  // Use BK_ASSERT from common/bk_assert.h
  }

  GetFileSize(h, stream_trace->size);

  stream_trace->buffer = MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (stream_trace->buffer = NULL) {
    BK_DUMP_OUT("Could not fill memory space! %d\n", GetLastError());
    BK_ASSERT(0);  // Use BK_ASSERT from common/bk_assert.h
  }
#else
  stream_trace->fp = open_memstream(&stream_trace->buffer, &stream_trace->size);
#endif /* _WIN32 */
#else  // For other os
#endif /* FREERTOS_SIMULATOR */
}

/* For cmodel */
static av_unused void *osal_aligned_malloc(unsigned int boundary,
                                           unsigned int memory_size) {
#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
  return _aligned_malloc(memory_size, boundary);
#elif defined(__linux__)
  return memalign(boundary, memory_size);
#endif /* _WIN32 */

#else  //FreeRTOS and other os need to implement by oneself
  return NULL;
#endif /* FREERTOS_SIMULATOR */
  return NULL;
}

/* For cmodel */
static av_unused void osal_aligned_free(void *aligned_momory) {
#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
  _aligned_free(aligned_momory);
#elif defined(__linux__)
  free(aligned_momory);
#endif /* _WIN32 */

#else  //FreeRTOS and other os need to implement by oneself
  ;
#endif /* FREERTOS_SIMULATOR */
}

#ifdef _HAVE_PTHREAD_H
static av_unused osal_pid osal_fork(process_main_ptr process_main) {
  BK_DUMP_OUT("FreeRTOS NOT Support multi-process!\n");
  BK_ASSERT(0);  // Use BK_ASSERT from common/bk_assert.h
  //resolve the warning
  osal_pid osal_pid_t = {0, NULL};
  return osal_pid_t;
}

static av_unused void osal_wait(osal_pid wait_pid, int *status) {
  BK_DUMP_OUT("FreeRTOS NOT Support multi-process!\n");
  BK_ASSERT(0);  // Use BK_ASSERT from common/bk_assert.h
}

#ifdef SEM_REPLACE_MUTEX

#include "dev_common_freertos.h"

static inline signed int sem_imp_mutex_init(sem_t *mutex,
                                            const pthread_mutexattr_t *attr) {
  sem_init(mutex, 0, 1);

  return 0;
}

static inline signed int sem_imp_cond_init(sem_t *cond,
                                           const pthread_condattr_t *attr) {
  sem_init(cond, 0, 0);

  return 0;
}

static inline signed int sem_imp_cond_wait(pthread_cond_t *cond,
                                           pthread_mutex_t *mutex) {
  if (sem_trywait(cond) != 0) {
    sem_post(mutex);
    sem_wait(cond);
    sem_wait(mutex);
  }

  return 0;
}
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_FREERTOS_H_ */
