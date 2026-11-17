/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VeriSilicon Inc                       --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--
--  Abstract : Hantro H1 JPEG Encoder API
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef __JPEGENCAPI_H__
#define __JPEGENCAPI_H__

#ifdef __cplusplus
extern "C" {
#endif


/* Included from base_type.h. */
/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2015 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------*/

#ifndef BASE_TYPE_H
#define BASE_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

// #include <stdint.h>
// //#include <stdio.h>
// #ifndef NDEBUG
// #include <assert.h>
// #endif
// #include <stdbool.h>

#include "os/os.h"

#ifndef SYSTEMSHARED
#define VSIAPI(x) VSIAPI##x /* redefine API with prefix "VSIAPI" */
#else
#define VSIAPI(x) VSISYS##x /* redefine API with prefix "VSISYS" */
#endif

/**
 * \defgroup base_type Numeric Data Types
 *
 * \brief This section provides the definitions of numeric data types.
 *
 * @{
 */
/** \brief Signed 8-bit integer. */
typedef int8_t i8;
/** \brief Unsigned 8-bit integer. */
typedef uint8_t u8;
/** \brief Signed 16-bit integer. */
typedef int16_t i16;
/** \brief Unsigned 16-bit integer. */
typedef uint16_t u16;
/** \brief Signed 32-bit integer. */
typedef int32_t i32;
/** \brief Unsigned 32-bit integer. */
// typedef uint32_t u32;
/** \brief Signed 64-bit integer. */
//typedef int64_t i64;
/** \brief Unsigned 64-bit integer. */
//typedef uint64_t u64;

typedef   signed long long i64;
typedef unsigned long long u64;

#ifdef __FREERTOS__

#ifndef FREERTOS_SIMULATOR
typedef unsigned long long
    ptr_t;  //Now the FreeRTOS Simulator just support the 64bit env
#else
typedef size_t ptr_t;
#endif

#else

/** \brief Buffer address. */
typedef size_t ptr_t;
#endif

/**@}*/

#ifndef INLINE
#define INLINE inline
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else /*  */
#define NULL ((void *)0)
#endif /*  */
#endif

typedef short Short;
typedef int Int;
typedef unsigned int UInt;

#ifndef __cplusplus

#define HANTRO_FALSE 0
#define HANTRO_TRUE 1

enum { OK = 0, NOK = -1 };
#endif

/* ASSERT */
#ifndef NDEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

#ifdef __cplusplus
}
#endif

#endif

/* Included from enccommon.h. */
/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Encoder common definitions for control code and system model
--
------------------------------------------------------------------------------*/

#ifndef __ENC_COMMON_H__
#define __ENC_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
    1. External compiler flags
------------------------------------------------------------------------------*/

/* Encoder global definitions
 *
 * _ASSERT_USED     # Asserts enabled
 * _DEBUG_PRINT     # Prints debug information on stdout
 * TEST_DATA        # Creates test data files
 *
 * Can be defined here or using compiler flags */

/*------------------------------------------------------------------------------
    2. Include headers
------------------------------------------------------------------------------*/


/* Included from base_type.h. */

/* Included from ewl.h. */
/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : H2 Encoder Wrapper Layer for OS services
--
------------------------------------------------------------------------------*/

#ifndef __EWL_H__
#define __EWL_H__

#ifdef __cplusplus
extern "C" {
#endif


/* Included from base_type.h. */

/* Included from vsi_queue.h. */
/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------*/

#ifndef QUEUE_H_
#define QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct node {
  struct node *next;
};

struct queue {
  struct node *head;
  struct node *tail;
};

/* rename functions with prefix. */
#define queue_init VSIAPI(queue_init)
#define queue_put VSIAPI(queue_put)
#define queue_put_tail VSIAPI(queue_put_tail)
#define queue_remove VSIAPI(queue_remove)
#define free_nodes VSIAPI(free_nodes)
#define queue_get VSIAPI(queue_get)
#define queue_tail VSIAPI(queue_tail)
#define queue_head VSIAPI(queue_head)

void queue_init(struct queue *);
void queue_put(struct queue *, struct node *);
void queue_put_tail(struct queue *, struct node *);
void queue_remove(struct queue *, struct node *);
void free_nodes(struct node *tail);
struct node *queue_get(struct queue *);
struct node *queue_tail(struct queue *queue);
struct node *queue_head(struct queue *queue);

#ifdef __cplusplus
}
#endif

#endif

/* Included from osal.h. */
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

#ifndef _OSAL_H_
#define _OSAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__FREERTOS__) || defined(_WIN32) || defined(__linux__)

#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif

typedef struct {
  beken_mutex_t mutex;
} vsios_mutex_t;

typedef struct {
  beken_semaphore_t sem;
} vsios_sem_t;

typedef struct {
  beken_thread_t thread;
} vsios_thread_t;

#ifdef __FREERTOS__

/* Included from osal_freertos.h. */
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

/* Included from vsi_string.h. */
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
#ifndef _VSI_STRING_H_
#define _VSI_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif

#ifdef SAFESTRING
//safestring imp for Intel
#include "safe_lib.h"
#include "snprintf_s.h"

#define strlen(dest) strnlen_s(dest, 512)
//#define wcslen(dest)              wcsnlen_s(dest, 512)

#define strcat(dest, src) strcat_s(dest, 512, src)
//#define strncat(dest,src,n)       strncat_s(dest, 512, src, n)

#define strcpy(dest, src) strcpy_s(dest, 512, src)
#define strncpy(dest, src, n) strncpy_s(dest, 512, src, n)

#define memcpy(dest, src, n) memcpy_s(dest, n, src, n)
#define memmove(dest, src, n) memmove_s(dest, n, src, n)
/* #define memcmp(s1,s2,n)           int ind = 0;\
                                  int rc = 0;\
                                  if (rc = memcmp_s(s1, 512, s2, n, &ind) != EOK ) {\
    	                            printf("%s %u  Ind=%d  Error rc=%u \n", __FUNCTION__, __LINE__, ind, rc);\
									assert(0);\
                                  }\
	                                return ind; */

#define memset(dest, value, n) memset_s(dest, n, value)

typedef errno_t mem_ret;
av_unused static mem_ret vsi_strcmp(const char *s1, const char *s2) {
  int ind = 0;
  int rc = 0;
  if ((rc = strcmp_s(s1, RSIZE_MAX_STR, s2, &ind)) != EOK) {
    printf("%s %d  Ind=%d  Error rc=%d \n", __FUNCTION__, __LINE__, ind, rc);
    assert(0);
  }

  return ind;
}

#define strcmp vsi_strcmp
//...
#else
/* For memset, strcpy and strlen */
#include <string.h>

#define snprintf_s_i(dest, n, format, a) snprintf(dest, n, format, a)
#define snprintf_s_si(dest, dmax, format, s, a) \
  snprintf(dest, dmax, format, s, a)
#define snprintf_s_l(dest, dmax, format, a) snprintf(dest, dmax, format, a)
#define snprintf_s_sl(dest, dmax, format, s, a) \
  snprintf(dest, dmax, format, s, a)
#define snprintf_s_s(dest, dmax, format, s) snprintf(dest, dmax, format, s)
#define snprintf_s_sssiii(dest, dmax, format, s1, s2, s3, inta1, inta2, inta3) \
  snprintf(dest, dmax, format, s1, s2, s3, inta1, inta2, inta3)

typedef void *mem_ret;

av_unused static int vsi_strcmp(const char *s1, const char *s2) {
  return strcmp(s1, s2);
}

#endif /* SAFESTRING */

#ifdef __cplusplus
}
#endif

#endif /* _VSI_STRING_H_ */

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
#elif defined(__linux__)

/* Included from osal_linux.h. */
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

#ifndef _OSAL_LINUX_H_
#define _OSAL_LINUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HAVE_PTHREAD_H
#define _HAVE_PTHREAD_H
#endif

#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>

#ifdef _HAVE_PTHREAD_H
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <unistd.h>
#endif /* _HAVE_PTHREAD_H */

#include <limits.h> /* NAME_MAX, CHAR_BIT*/
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>

typedef struct timeval osal_timeval;
typedef struct timezone osal_timezone;

typedef void *(*process_main_ptr)(void *arg);

struct stream_trace {
  struct node *next;
  char *buffer;
  char comment[256];
  size_t size;
  FILE *fp;
  unsigned int cnt;
};

typedef struct {
  int pid;
  pthread_t *tid;
} osal_pid;

#define _FILE_OFFSET_BITS 64  // for 64 bit fseeko
#define fseek fseeko
#define int16 int16_t
#define int64 int64_t
#define __int64 long long

#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif

static av_unused int osal_gettimeofday(osal_timeval *tp, void *tzp) {
  return gettimeofday(tp, (osal_timezone *)tzp);
}

static av_unused void osal_usleep(unsigned long usec) {
  if (usec == 0)
    sched_yield();
  else
    usleep(usec);
}

static av_unused void osal_open_memstream(struct stream_trace *stream_trace) {
  stream_trace->fp = open_memstream(&stream_trace->buffer, &stream_trace->size);
}

static av_unused void *osal_aligned_malloc(unsigned int boundary,
                                           unsigned int memory_size) {
  return memalign(boundary, memory_size);
}

static av_unused void osal_aligned_free(void *aligned_momory) {
  free(aligned_momory);
}

#ifdef _HAVE_PTHREAD_H
static av_unused osal_pid osal_fork(process_main_ptr process_main) {
  int pid;
  osal_pid osal_pid_t;
  osal_pid_t.tid = NULL;
  if (0 == (pid = fork())) {
    process_main(NULL);
    exit(0);
  } else if (pid > 0) {
    osal_pid_t.pid = pid;
  } else {
    perror("failed to fork new process to process streams");
    exit(pid);
  }
  printf("osal_pid_t.osal_pid is %d\n", osal_pid_t.pid);
  return osal_pid_t;
}

static av_unused void osal_wait(osal_pid wait_pid, int *status) {
  waitpid(wait_pid.pid, status, 0);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_LINUX_H_ */
#elif defined(_WIN32)

/* Included from osal_win32.h. */
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

#ifndef _OSAL_WIN32_H_
#define _OSAL_WIN32_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HAVE_PTHREAD_H
#define _HAVE_PTHREAD_H
#endif

#ifdef _HAVE_PTHREAD_H
#include "pthread.h"
#include "semaphore.h"
#include "sched.h"
#endif /* _HAVE_PTHREAD_H */

#include <io.h>
#include <process.h>
#include <windows.h>
#include <time.h>

#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>

#if defined(_MSC_VER)
/* MSVS doesn't define off_t, and uses _f{seek,tell}i64. */
typedef __int64 off_t;
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
#endif
#define snprintf _snprintf
#define open _open
#define close _close
#define read _read
#define write _write
//#define lseek _lseeki64
//#define fsync _commit
//#define tell _telli64

#define NAME_MAX 255
#undef fseek
#define fseek _fseeki64
#define int16 __int16
#define int64 __int64

#define putw _putw

#ifndef CMODEL_DLL_API
#ifdef CMODEL_DLL_SUPPORT /* cmodel in dll */
#ifdef _CMODEL_ /* This macro will be enabled when compiling cmodel libs. */
#define CMODEL_DLL_API __declspec(dllexport)
#else
#define CMODEL_DLL_API __declspec(dllimport)
#endif
#else
/* win non-dll. */
#define CMODEL_DLL_API
#endif
#endif

typedef unsigned long(__stdcall *THREAD_ROUTINE)(void *Argument);
typedef void *(*process_main_ptr)(void *arg);

typedef struct timeval osal_timeval;

struct stream_trace {
  struct node *next;
  char *buffer;
  char comment[256];
  size_t size;
  FILE *fp;
  unsigned int cnt;
};

typedef struct {
  int pid;
  pthread_t *tid;
} osal_pid;

static av_unused int osal_gettimeofday(osal_timeval *tp, void *tzp) {
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
}

static av_unused void osal_usleep(unsigned long usec) {
  if (usec < 1000)
    Sleep(10);
  else
    Sleep(usec / 1000);
}

static av_unused void osal_open_memstream(struct stream_trace *stream_trace) {
  int fd;
  HANDLE fm, h;
  fd = _fileno(stream_trace->fp);
  h = (HANDLE)_get_osfhandle(fd);

  fm = CreateFileMapping(h, NULL, PAGE_READWRITE | SEC_RESERVE, 0,
                         16 * 1024 * 1024, NULL);
  if (fm == NULL) {
    fprintf(stderr, "Could not access memory space! %sn",
            strerror(GetLastError()));
    exit(GetLastError());
  }

  GetFileSize(h, stream_trace->size);

  stream_trace->buffer = MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (stream_trace->buffer = NULL) {
    fprintf(stderr, "Could not fill memory space! %sn",
            strerror(GetLastError()));
    exit(GetLastError());
  }
}

static av_unused void *osal_aligned_malloc(unsigned int boundary,
                                           unsigned int memory_size) {
  return _aligned_malloc(memory_size, boundary);
}

static av_unused void osal_aligned_free(void *aligned_momory) {
  _aligned_free(aligned_momory);
}

#ifdef _HAVE_PTHREAD_H
static av_unused osal_pid osal_fork(process_main_ptr process_main) {
  osal_pid osal_pid_t;
  osal_pid_t.pid = 0;
  pthread_t *ptid = (pthread_t *)malloc(sizeof(pthread_t));
  pthread_create(ptid, NULL, process_main, NULL);
  osal_pid_t.tid = ptid;
  return osal_pid_t;
}

static av_unused void osal_wait(osal_pid wait_pid, int *status) {
  pthread_join(*wait_pid.tid, NULL);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_WIN32_H_ */
#endif

#ifdef _HAVE_PTHREAD_H
//For static link pthread in win32, need init some global variables to prevent crash
#ifdef PTW32_STATIC_LIB
static void detach_ptw32(void) {
  pthread_win32_thread_detach_np();
  pthread_win32_process_detach_np();
}
#endif

static av_unused void osal_thread_init() {
#ifdef PTW32_STATIC_LIB
  pthread_win32_process_attach_np();
  pthread_win32_thread_attach_np();
  atexit(detach_ptw32);
#endif
}
#endif /* _HAVE_PTHREAD_H */

#endif /* defined(__FREERTOS__) || defined(__linux__) || defined(_WIN32) */

#define gettimeofday osal_gettimeofday
#define usleep osal_usleep
//#define open_memstream osal_open_memstream

#ifdef __linux__
#define OSAL_STRM_POS(A) (A).__pos
#else
#define OSAL_STRM_POS(A) (A)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_H_ */
// #include "encswhwregisters.h"

/* Included from vsi_string.h. */


/* Included from ewl_hwcfg.h. */
/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--      In the event of publication, the following notice is applicable:      --
--                                                                            --
--                   (C) COPYRIGHT 2020 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--         The entire notice above must be reproduced on all copies.          --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Encoder HW Configure features. 
--
------------------------------------------------------------------------------*/

/* Genenrated on 2024-05-31, not modify. */

#ifndef EWL_HWCFG_H
#define EWL_HWCFG_H

#ifdef __cplusplus
extern "C" {
#endif


/* Included from base_type.h. */

/** \brief The hardware configuration information. */
typedef struct EWLHwConfig {
    //Sub-System
    /** \brief The ASIC_ID to identify the hardware version. */
    u32 hw_asic_id;
    /** \brief The BUILD_ID of the hardware. */
    u32 hw_build_id;
    //Format
    /** \brief Whether the hardware supports HEVC encoding. \n 0: Does not support. \n 1: Supports. */
    u32 hevcEnabled;
    /** \brief Whether the hardware supports H.264 encoding. \n 0: Does not support. \n 1: Supports. */
    u32 h264Enabled;
    /** \brief Whether the hardware supports JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 jpegEnabled;
    /** \brief Whether the hardware supports VP9 encoding. \n 0: Does not support. \n 1: Supports. */
    u32 vp9Enabled;
    /** \brief Whether the hardware supports AV1 encoding. \n 0: Does not support. \n 1: Supports. */
    u32 av1Enabled;
    //IM
    /** \brief Whether the hardware supports CuTree look-ahead. \n 0: Does not support. \n 1: Supports. */
    u32 cuTreeSupport;
    //Resolution
    /** \brief The maximum video width supported by the hardware for HEVC encoding, in pixels. */
    u32 maxEncodedWidthHEVC;
    /** \brief The maximum video width supported by the hardware for H.264 encoding, in pixels. */
    u32 maxEncodedWidthH264;
    /** \brief The maximum video width supported by the hardware for AV1 encoding, in pixels. */
    u32 maxEncodedWidthAV1;
    /** \brief The maximum video width supported by the hardware for VP9 encoding, in pixels. */
    u32 maxEncodedWidthVP9;
    /** \brief The maximum video height. \n 0: 8192 pixels.  \n 1: 8640 pixels. */
    u32 videoHeightExt;
    /** \brief The maximum video width supported by the hardware for JPEG encoding, in pixels. */
    u32 maxEncodedWidthJPEG;
    //Major 
    /** \brief Whether the hardware supports B-frame. \n 0: Does not support. \n 1: Supports. */
    u32 bFrameEnabled;
    /** \brief Whether the hardware supports main 10 or main 8. \n 0: Supports main 8. \n 1: Supports main 10. */
    u32 main10Enabled;
    /** \brief Whether the hardware supports H.264 rate distorted optimized quantization (RDOQ). \n 0: Does not support. \n 1: Supports. */
    u32 RDOQSupportH264;
    /** \brief Whether the hardware supports HEVC RDOQ. \n 0: Does not support. \n 1: Supports. */
    u32 RDOQSupportHEVC;
    /** \brief Whether the hardware supports AV1 RDOQ. \n 0: Does not support. \n 1: Supports. */
    u32 RDOQSupportAV1;
    /** \brief Whether the hardware supports VP9 RDOQ. \n 0: Does not support. \n 1: Supports. */
    u32 RDOQSupportVP9;
    /** \brief The horizontal search range of motion estimation (ME).
     * \n 0: 128 pixels for P-frame, 64 pixels for B-frame.
     * \n 1: 128 pixels for P-frame, 112 pixels for B-frame.
     * \n 2: 192 pixels for P-frame, 112 pixels for B-frame.
     * \n 3: 240 pixels for P-frame, 240 pixels for B-frame.
     * \n 4: 112 pixels for P-frame, 56 pixels for B-frame. */
    u32 meHorSearchRange;
    /** \brief The ME vertical search range for H.264. */
    u32 meVertSearchRangeH264;
    /** \brief The ME vertical Search range for HEVC, AV1, and VP9. */
    u32 meVertSearchRangeHEVC;
    /** \brief Whether the hardware supports SSIM calculation for HEVC and H.264. \n 0: Does not support. \n 1: Supports. */
    u32 ssimSupport;
    /** \brief Whether the hardware supports PSNR calculation for HEVC and H.264. \n 0: Does not support. \n 1: Supports. */
    u32 psnrSupport;
    /** \brief Whether the hardware supports SSIM calculation for AV1. \n 0: Does not support. \n 1: Supports. */
    u32 ssimSupportAV1;
    /** \brief Whether the hardware supports PSNR calculation for AV1. \n 0: Does not support. \n 1: Supports. */
    u32 psnrSupportAV1;
    /** \brief Whether the hardware supports HEVC tile. \n 0: Does not support. \n 1: Supports. */
    u32 hevcTileSupport;
    /** \brief Whether the hardware supports HEVC SAO. \n 0: Does not support. \n 1: Supports. */
    u32 saoSupport;
    //Minor
    /** \brief Whether the hardware supports reference frame compressor (RFC). \n 0: Does not support. \n 1: Supports. */
    u32 rfcEnable;
    /** \brief the pixel block size controlled by each Qp
0: 8x8   1: 16x16 */
    u32 roiBlkSize;
    /** \brief The hardware version for ROI map. \n For more information, see <i>Hantro VC9000E Series Memory Buffer and Format Organization</i>. */
    u32 roiMapVersion;
    /** \brief The output format of CU statistics information. \n For more information, see <i>Hantro VC9000E Series Memory Buffer and Format Organization</i>. */
    u32 cuInforVersion;
    /** \brief Whether the hardware supports CTB-level or MB-level rate control. \n 0: Does not support. \n 1: Support. */
    u32 ctbRcSupport;
    /** \brief The CTB rate control version supported by hardware.
     * \n 0: The version only supports to adjust QP for subjective quality.
     * \n 1: The version supports to adjust QP for subjective quality and target bits.
     * \n 2: The version supports to adjust QP for subjective quality and target bits with a larger range of adjustment. */
    u32 ctbRcVersion;
    /** \brief Whether the hardware supports frame-level information output. \n 0: Does not support. \n 1: Supports. */
    u32 frameInfoSupport;
    /** \brief Whether the hardware supports programmable RDO level.  \n 0: Does not support. \n 1: Supports. */
    u32 progRdoEnable;
    /** \brief Whether the hardware supports JPEG encoding with lossless process. \n 0: Does not support. \n 1: Supports. */
    u32 ljpegSupport;
    /** \brief Whether the hardware supports ROI map for JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 JpegRoiMapSupport;
    /** \brief Whether the hardware supports monochrome JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 jpeg400Support;
    /** \brief Whether the hardware supports YUV422 JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 jpeg422Support;
    /** \brief Whether the hardware supports Huffman table (DHT) generation. \n 0: Does not support. \n 1: Supports. */
    u32 JpegDHTGenerate;
    /** \brief Whether the hardware supports user-defined Huffman table for JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 JpegExternalDHT;
    /** \brief Whether the hardware supports programmable vertical ME range. \n 0: Does not support. \n 1: Supports */
    u32 meVertRangeProgramable;
    /** \brief Whether the hardware supports monochrome H.264 and HEVC encoding. \n 0: Does not support. \n 1: Supports. */
    u32 MonoChromeSupport;
    /** \brief Whether the hardware supports HEVC YUV422. \n 0: Does not support. \n 1: Supports. */
    u32 HevcYUV422Support;
    /** \brief Whether the hardware supports H.264 YUV422. \n 0: Does not support. \n 1: Supports. */
    u32 H264YUV422Support;
    /** \brief Whether the hardware supports HEVC YUV444. \n 0: Does not support. \n 1: Supports. */
    u32 HevcYUV444Support;
    /** \brief Whether the hardware supports H.264 YUV444. \n 0: Does not support. \n 1: Supports. */
    u32 H264YUV444Support;
    /** \brief Whether the hardware supports 32x32 intra TU. \n 0: Does not support. \n 1: Supports. */
    u32 intraTU32Enable;
    /** \brief Whether the hardware supports look-ahead. \n 0: Does not support. \n 1: Supports. */
    u32 bMultiPassSupport;
    /** \brief Whether the hardware supports 32x32 inter TU. \n 0: Does not support. \n 1: Supports. */
    u32 tu32Enable;
    /** \brief Whether the hardware supports dynamic maximum TU size. \n 0: Does not support \n 1: Supports. */
    u32 dynamicMaxTuSize;
    /** \brief Whether the hardware supports number of bits output for each CTB. \n 0: Does not support. \n 1: Supports. */
    u32 CtbBitsOutSupport;
    /** \brief Whether the hardware supports 32x32 inter TU. \n 0: Does not support. \n 1: Supports. */
    u32 encVisualTuneSupport;
    /** \brief Whether the hardware supports revert quality adjustment when the subjective CTB rate control is enabled. \n 0: Does not support. \n 1: Supports. */
    u32 ctbRcMoreMode;
    /** \brief Whether the hardware supports denoise. \n 0: Does not support. \n 1: Supports. */
    u32 deNoiseEnabled;
    /** \brief Whether use input down sampled pixel as Ref of ME4N. \n 0: Does not support. \n 1: Supports. */
    u32 encRef4NInputSupport;
    /** \brief Whether the hardware supports tuning quality according to psy-rd. \n 0: Does not support. \n 1: Supports. */
    u32 encPsyTuneSupport;
    /** \brief Whether the hardware supports motion score calculation and the storage of the result into a software register. \n 0: Does not support. \n 1: Supports. */
    u32 motionScoreEnable;
    /** \brief Whether the hardware supports SSE calculation for recon pixel. */
    u32 reconSSEsupport;
    /** \brief The maximum active reference frame number for P-frame encoding. \n 0: One reference frame. \n 1: Two reference frames. */
    u32 maxRefNumList0Minus1;
    /** \brief The maximum slice height of CTBs or MBs supported by the hardware. \n If the value is n, then the maximum height is 2 to the Power(2,n)-1. */
    u32 encSliceSizeBits;
    /** \brief Whether the hardware supports asymmetric PU partition for HEVC. \n 0: Does not support. \n 1: Supports. */
    u32 asymPuSupport;
    /** \brief Whether the hardware supports the fourth skip candidate for HEVC and H.264. \n 0: Does not support. \n 1: Supports. */
    u32 maxMergeCand4Support;
    /** \brief Whether the hardware supports outputting adaptive quantization information. \n 0: Does not support. \n 1: Supports. */
    u32 encAQInfoOut;
    //PRP
    /** \brief Whether the hardware supports RGB-to-YUV conversion. \n 0: Does not support. \n 1: Supports. */
    u32 rgbEnabled;
    /** \brief Whether the hardware supports rotation in pre-processing. \n 0: Supports. \n 1: Does not support. */
    u32 NonRotationSupport;
    /** \brief Whether the hardware only supports NV12 as input. \n 0: Supports other format. \n 1: Only supports NV12. */
    u32 NVFormatOnlySupport;
    /** \brief Whether the hardware supports low-latency mechnism on input picture. \n 0: Supports. \n 1: Does not support. */
    u32 prpLowLatencySupport;
    /** \brief Pre-Processing use sync word in DDR to do low latency handshake. <tt>prpLowLatencySupport</tt> should be <tt>1</tt> if this feature is selected. \n 0: Supports. \n 1: Does not support. */
    u32 prpLowlatencySignalbyDDR;
    /** \brief Whether the hardware supports in-loop down-scaler. \n 0: Does not support. \n 1: Supports. */
    u32 inLoopDSRatio;
    /** \brief Whether the hardware supports full or partial range conversion when implementing RGB-to-YUV color conversion. \n 0: Does not support. \n 1: Supports. */
    u32 cscExtendSupport;
    /** \brief Whether the hardware supports down-scaling. \n 0: Does not support. \n 1: Supports. */
    u32 scalingEnabled;
    /** \brief Whether the hardware supports out-loop scaler output in YUV420SP format. \n 0: Does not support. \n 1: Supports. */
    u32 scaled420Support;
    /** \brief Whether the hardware supports video stabilization. \n 0: Does not support. \n 1: Supports. */
    u32 vsSupport;
    /** \brief Whether the hardware supports OSD. \n 0: Does not support. \n 1: Supports. */
    u32 OSDSupport;
    /** \brief The maximum number of OSD rectangles supported by the hardware. */
    u32 maxOsdNum;
    /** \brief Whether the hardware supports mosaic. \n 0: Does not support. \n 1: Supports. */
    u32 MosaicSupport;
    /** \brief The number of mosaic rectangles supported by the hardware. */
    u32 maxMosaicNum;
    /** \brief The version of of mosaic. \n 0: 64x64 for HEVC, 16x16 for H264. \n 1: 8x8/16x16/32x32/64x64/128x128/ for H264 and HEVC. */
    u32 mosaicVersion;
    /** \brief Hardware OSD alpha blending Version in the pre-processing pipeline. \n 0: Does not support. \n 1: Supports. */
    u32 osdBlendVersion;
    /** \brief Whether the hardware supports picture mosaic control by map buffer in DDR or not. The mosaic information of every 8x8 blocks are specified in the map buffer. \n 0: Does not support. \n 1: Supports. */
    u32 OSDMapSupport;
    /** \brief The OSD Map version. \n 0: 4bits for each 8x8. \n 1: 4 bits for each 4x4. */
    u32 OSDMapVersion;
    /** \brief Whether the hardware supports 8x8 tiled input formats.
     * \n For more information about input formats, refer to <i>Hantro VC9000E v1x Hardware Features</i>. */
    u32 tile8x8FormatSupport;
    /** \brief Whether the hardware supports 4x4 tiled input formats.
     * \n For more information about input formats, refer to <i>Hantro VC9000E v1x Hardware Features</i>. */
    u32 tile4x4FormatSupport;
    /** \brief Whether the hardware supports tiled input formats.
     * \n For more information about input formats, refer to <i>Hantro VC9000E v1x Hardware Features</i>. */
    u32 customTileFormatSupport;
    /** \brief Whether the hardware supports RGB888 or BGR888 input formats. \n 0: Does not support. \n 1: Supports. */
    u32 rgb24bitFormatSupport;
    /** \brief Whether the hardware supports FLEXA SBI in the pre-processing pipeline. \n 0: Does not support. \n 1: Supports. */
    u32 prpSbiSupport;
    /** \brief 100: AFBC
200: PVRIC
400: DEC400 */
    u32 ufbcSupport;
    /** \brief Whether the pre-processing module supports the X-major super tile input format that is output by VeriSilicon GPU IPs. \n 0: Does not support. \n 1: Supports */
    u32 superTileXSupport;
    /** \brief Whether the pre-processing module supports input formats YUV420SP8b_Tile64x4_A16N and YVU420SP8b_Tile64x4_A16N. \n 0: Does not support. \n 1: Supports. */
    u32 tile64x4_8bFormatSupport;
    /** \brief Whether the pre-processing module supports input format YUV420SP10bWH_Tile32x4_A16N. */
    u32 tile32x4_10bFormatSupport;
    /** \brief Whether the hardware supports input analyzer in the pre-processing pipeline. \n 0: Does not support. \n 1: Supports. */
    u32 prpAnalyzerSupport;
    /** \brief Whether the hardware supports mirror in pre-processing pipeline. \n 0: Does not support. \n 1: Supports. */
    u32 prpMirrorSupport;
    //HW
    /** \brief Whether the hardware supports external SRAM. \n 0: Does not support. \n 1: Supports. */
    u32 meExternSramSupport;
    /** \brief The bus type of the hardware. \n 1: AHB. \n 2: OCP. \n 3: AXI. \n 4: PCI. \n 5: AXI and AHB. \n 6: AXI and APB. */
    u32 busType;
    /** \brief The bus width. \n The value is fixed to 2, which indicates 128 bits. */
    u32 busWidth;
    /** \brief Reserved. */
    u32 maxAXIAlignment;
    /** \brief AXI alignment setting for writing common data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_common;
    /** \brief AXI alignment setting for writing bit streams. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_stream;
    /** \brief AXI alignment setting for writing chroma reconstructed frame data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_chroma_ref;
    /** \brief AXI alignment setting for writing luma reconstructed frame data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_luma_ref;
    /** \brief AXI alignment setting for reading common data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_rd_common;
    /** \brief AXI alignment setting for reading pre-processing data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_rd_prp;
    /** \brief AXI alignment setting for reading chroma reference frame data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_rd_ch_ref_prefetch;
    /** \brief AXI alignment setting for reading luma reference frame data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_rd_lu_ref_prefetch;
    /** \brief AXI alignment setting for writing CU information. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_cuinfo;
    /** \brief Whether to have SW registers to control the clock gating behavior when low latency hanshake stall the encoding pipeline */
    u32 lowLatClkGate;
    //supported in most HW (keep them always supported in future HW)
    /** \brief The number of bits used to write the nal_ref_idc syntax element of H.264. \n 0: 1 bit \n 1: 2 bits. */
    u32 h264NalRefIdc2bit;
    /** \brief Whether the hardware supports the av1_extension_flag register which controls the syntax in OBU_Header. \n 0: Not supported. \n 1: Supported. */
    u32 av1ExtensionFlag;
    /** \brief Whether the hardware supports ROIs with absolute QP. \n 0: Does not support.  \n 1: Supports. */
    u32 roiAbsQpSupport;
    /** \brief Whether the hardware supports two or eight ROIs. \n 0: Supports two ROIs.  \n 1: Supports eight ROIs. */
    u32 ROI8Support;
    /** \brief Whether the hardware supports context-adaptive variable-length coding (CAVLC). \n 0: Does not support. \n 1: Supports. */
    u32 h264CavlcEnable;
    /** \brief Whether the hardware supports linear or ring structure for reference and reconstructed frame buffers for P-frame encoding.
     *  \n 0: Only supports linear structure. \n 1: Supports both linear and ring structure. */
    u32 refRingBufEnable;
    /** \brief Whether the hardware supports disabling reconstructed frame writing. \n 0: Does not support. \n 1: Supports. */
    u32 disableRecWtSupport;
    /** \brief Whether the hardware uses CU size map from IntraModeSearch block (not decided by PK inter vs inter rdcost) for the Cus which are forced to Intra by GDR/CIR/ROI. \n 0: Does not support. \n 1: Supports. */
    u32 forceIntraCuSizeOpt;
    //Not supported in most HW
    /** \brief Whether the hardware supports background detection. \n 0: Does not support. \n 1: Supports. (Only for a few customers) */
    u32 backgroundDetSupport;
    /** \brief Whether the hardware supports using P010 as the reference frame format for 10-bit encoding.  \n 0: Does not support. \n 1: Supports. (Only for a few customers)  */
    u32 P010RefSupport;
    /** \brief Whether the hardware supports stream buffer chain. \n 0: Does not support. \n 1: Supports. */
    u32 streamBufferChain;
    /** \brief Whether the hardware supports global motion vector. \n 0: Does not support. \n 1: Supports. (Only for a few customers) */
    u32 gmvSupport;
    /** \brief Whether the hardware supports eight IPCM area setting. \n 0: Does not support. \n 1: Supports. */
    u32 IPCM8Support;
    //Not implemented in HW
    /** \brief Whether the hardware supports AV1 switchable interpolation filter. \n 0: Does not support. \n 1: Supports. (Only for C-model) */
    u32 av1InterpFilterSwitchable;
    /** \brief Whether the hardware supports I-frame encoding only. \n 0: Does not support. \n 1: Supports. */
    u32 IframeOnly;
    /** \brief Whether the hardware supports dynamical choosing of RDO level. \n 0: Does not support. \n 1: Supports. */
    u32 dynamicRdoSupport;
    /** \brief Whether the hardware supports quality improvement defined in set two. \n 0: Does not support. \n 1: Supports. */
    u32 tuneToolsSet2Support;
    /** \brief Whether the hardware supports TMVP for HEVC. \n 0: Does not support. \n 1: Supports. */
    u32 hevcTemporalMvpSupport;
    /** \brief Whether the hardware supports TMVP for AV1. \n 0: Does not support. \n 1: Supports. */
    u32 av1TemporalMvpSupport;
    /** \brief Whether the hardware supports TMVP with multiple cores together. \n 0: Does not support. \n 1: Supports. */
    u32 tmvpMcSupport;
    /** \brief Whether the hardware supports intra mode search with reconstructed frames.  \n 0: Does not support. \n 1: Supports. */
    u32 intraReconSupport;
    /** \brief Whether the hardware supports AV1 intra mode search with reconstructed frames.  \n 0: Does not support. \n 1: Supports. */
    u32 av1IntraReconSupport;
    /** \brief Whether the hardware supports the CuTree RDO architecture for HEVC.  \n 0: Does not support. \n 1: Supports. */
    u32 hevcCuRdoSupport;
    /** \brief Whether the hardware supports the CuTree RDO architecture for AV1.  \n 0: Does not support. \n 1: Supports. */
    u32 av1CuRdoSupport;
    /** \brief Whether the hardware supports simplified cost instead of Meqn binCost.   \n 0: Does not support. \n 1: Supports. */
    u32 meqnSimpleBinCost;
    /** \brief Whether the hardware supports multiple segments for stream buffer. \n 0: Does not support. \n 1: Supports. */
    u32 streamMultiSegment;
    /** \brief Whether the hardware supports the modification of subjective quality improvement for H.264. \n 0: Does not support. \n 1: Supports. */
    u32 modSubjPrefer;
    /** \brief Whether the hardware supports both the 2RefP and TMVP. \n 0: Does not support. \n 1: Supports. */
    u32 twoRefPTMVPSupport;
}EWLHwConfig_t;

#define EWLShowHwConfig(info) \
  do { \
    PTRACE("Get Hantro Encoder Hardware Configure:\n"); \
    PTRACE("                               hw_asic_id = 0x%x\n", info ->hw_asic_id); \
    PTRACE("                              hw_build_id = 0x%x\n", info ->hw_build_id); \
    PTRACE("                              hevcEnabled = %d\n", info ->hevcEnabled); \
    PTRACE("                              h264Enabled = %d\n", info ->h264Enabled); \
    PTRACE("                              jpegEnabled = %d\n", info ->jpegEnabled); \
    PTRACE("                               vp9Enabled = %d\n", info ->vp9Enabled); \
    PTRACE("                               av1Enabled = %d\n", info ->av1Enabled); \
    PTRACE("                            cuTreeSupport = %d\n", info ->cuTreeSupport); \
    PTRACE("                      maxEncodedWidthHEVC = %d\n", info ->maxEncodedWidthHEVC); \
    PTRACE("                      maxEncodedWidthH264 = %d\n", info ->maxEncodedWidthH264); \
    PTRACE("                       maxEncodedWidthAV1 = %d\n", info ->maxEncodedWidthAV1); \
    PTRACE("                       maxEncodedWidthVP9 = %d\n", info ->maxEncodedWidthVP9); \
    PTRACE("                           videoHeightExt = %d\n", info ->videoHeightExt); \
    PTRACE("                      maxEncodedWidthJPEG = %d\n", info ->maxEncodedWidthJPEG); \
    PTRACE("                            bFrameEnabled = %d\n", info ->bFrameEnabled); \
    PTRACE("                            main10Enabled = %d\n", info ->main10Enabled); \
    PTRACE("                          RDOQSupportH264 = %d\n", info ->RDOQSupportH264); \
    PTRACE("                          RDOQSupportHEVC = %d\n", info ->RDOQSupportHEVC); \
    PTRACE("                           RDOQSupportAV1 = %d\n", info ->RDOQSupportAV1); \
    PTRACE("                           RDOQSupportVP9 = %d\n", info ->RDOQSupportVP9); \
    PTRACE("                         meHorSearchRange = %d\n", info ->meHorSearchRange); \
    PTRACE("                    meVertSearchRangeH264 = %d\n", info ->meVertSearchRangeH264); \
    PTRACE("                    meVertSearchRangeHEVC = %d\n", info ->meVertSearchRangeHEVC); \
    PTRACE("                              ssimSupport = %d\n", info ->ssimSupport); \
    PTRACE("                              psnrSupport = %d\n", info ->psnrSupport); \
    PTRACE("                           ssimSupportAV1 = %d\n", info ->ssimSupportAV1); \
    PTRACE("                           psnrSupportAV1 = %d\n", info ->psnrSupportAV1); \
    PTRACE("                          hevcTileSupport = %d\n", info ->hevcTileSupport); \
    PTRACE("                               saoSupport = %d\n", info ->saoSupport); \
    PTRACE("                                rfcEnable = %d\n", info ->rfcEnable); \
    PTRACE("                               roiBlkSize = %d\n", info ->roiBlkSize); \
    PTRACE("                            roiMapVersion = %d\n", info ->roiMapVersion); \
    PTRACE("                           cuInforVersion = %d\n", info ->cuInforVersion); \
    PTRACE("                             ctbRcSupport = %d\n", info ->ctbRcSupport); \
    PTRACE("                             ctbRcVersion = %d\n", info ->ctbRcVersion); \
    PTRACE("                         frameInfoSupport = %d\n", info ->frameInfoSupport); \
    PTRACE("                            progRdoEnable = %d\n", info ->progRdoEnable); \
    PTRACE("                             ljpegSupport = %d\n", info ->ljpegSupport); \
    PTRACE("                        JpegRoiMapSupport = %d\n", info ->JpegRoiMapSupport); \
    PTRACE("                           jpeg400Support = %d\n", info ->jpeg400Support); \
    PTRACE("                           jpeg422Support = %d\n", info ->jpeg422Support); \
    PTRACE("                          JpegDHTGenerate = %d\n", info ->JpegDHTGenerate); \
    PTRACE("                          JpegExternalDHT = %d\n", info ->JpegExternalDHT); \
    PTRACE("                   meVertRangeProgramable = %d\n", info ->meVertRangeProgramable); \
    PTRACE("                        MonoChromeSupport = %d\n", info ->MonoChromeSupport); \
    PTRACE("                        HevcYUV422Support = %d\n", info ->HevcYUV422Support); \
    PTRACE("                        H264YUV422Support = %d\n", info ->H264YUV422Support); \
    PTRACE("                        HevcYUV444Support = %d\n", info ->HevcYUV444Support); \
    PTRACE("                        H264YUV444Support = %d\n", info ->H264YUV444Support); \
    PTRACE("                          intraTU32Enable = %d\n", info ->intraTU32Enable); \
    PTRACE("                        bMultiPassSupport = %d\n", info ->bMultiPassSupport); \
    PTRACE("                               tu32Enable = %d\n", info ->tu32Enable); \
    PTRACE("                         dynamicMaxTuSize = %d\n", info ->dynamicMaxTuSize); \
    PTRACE("                        CtbBitsOutSupport = %d\n", info ->CtbBitsOutSupport); \
    PTRACE("                     encVisualTuneSupport = %d\n", info ->encVisualTuneSupport); \
    PTRACE("                            ctbRcMoreMode = %d\n", info ->ctbRcMoreMode); \
    PTRACE("                           deNoiseEnabled = %d\n", info ->deNoiseEnabled); \
    PTRACE("                     encRef4NInputSupport = %d\n", info ->encRef4NInputSupport); \
    PTRACE("                        encPsyTuneSupport = %d\n", info ->encPsyTuneSupport); \
    PTRACE("                        motionScoreEnable = %d\n", info ->motionScoreEnable); \
    PTRACE("                          reconSSEsupport = %d\n", info ->reconSSEsupport); \
    PTRACE("                     maxRefNumList0Minus1 = %d\n", info ->maxRefNumList0Minus1); \
    PTRACE("                         encSliceSizeBits = %d\n", info ->encSliceSizeBits); \
    PTRACE("                            asymPuSupport = %d\n", info ->asymPuSupport); \
    PTRACE("                     maxMergeCand4Support = %d\n", info ->maxMergeCand4Support); \
    PTRACE("                             encAQInfoOut = %d\n", info ->encAQInfoOut); \
    PTRACE("                               rgbEnabled = %d\n", info ->rgbEnabled); \
    PTRACE("                       NonRotationSupport = %d\n", info ->NonRotationSupport); \
    PTRACE("                      NVFormatOnlySupport = %d\n", info ->NVFormatOnlySupport); \
    PTRACE("                     prpLowLatencySupport = %d\n", info ->prpLowLatencySupport); \
    PTRACE("                 prpLowlatencySignalbyDDR = %d\n", info ->prpLowlatencySignalbyDDR); \
    PTRACE("                            inLoopDSRatio = %d\n", info ->inLoopDSRatio); \
    PTRACE("                         cscExtendSupport = %d\n", info ->cscExtendSupport); \
    PTRACE("                           scalingEnabled = %d\n", info ->scalingEnabled); \
    PTRACE("                         scaled420Support = %d\n", info ->scaled420Support); \
    PTRACE("                                vsSupport = %d\n", info ->vsSupport); \
    PTRACE("                               OSDSupport = %d\n", info ->OSDSupport); \
    PTRACE("                                maxOsdNum = %d\n", info ->maxOsdNum); \
    PTRACE("                            MosaicSupport = %d\n", info ->MosaicSupport); \
    PTRACE("                             maxMosaicNum = %d\n", info ->maxMosaicNum); \
    PTRACE("                            mosaicVersion = %d\n", info ->mosaicVersion); \
    PTRACE("                          osdBlendVersion = %d\n", info ->osdBlendVersion); \
    PTRACE("                            OSDMapSupport = %d\n", info ->OSDMapSupport); \
    PTRACE("                            OSDMapVersion = %d\n", info ->OSDMapVersion); \
    PTRACE("                     tile8x8FormatSupport = %d\n", info ->tile8x8FormatSupport); \
    PTRACE("                     tile4x4FormatSupport = %d\n", info ->tile4x4FormatSupport); \
    PTRACE("                  customTileFormatSupport = %d\n", info ->customTileFormatSupport); \
    PTRACE("                    rgb24bitFormatSupport = %d\n", info ->rgb24bitFormatSupport); \
    PTRACE("                            prpSbiSupport = %d\n", info ->prpSbiSupport); \
    PTRACE("                              ufbcSupport = %d\n", info ->ufbcSupport); \
    PTRACE("                        superTileXSupport = %d\n", info ->superTileXSupport); \
    PTRACE("                 tile64x4_8bFormatSupport = %d\n", info ->tile64x4_8bFormatSupport); \
    PTRACE("                tile32x4_10bFormatSupport = %d\n", info ->tile32x4_10bFormatSupport); \
    PTRACE("                       prpAnalyzerSupport = %d\n", info ->prpAnalyzerSupport); \
    PTRACE("                         prpMirrorSupport = %d\n", info ->prpMirrorSupport); \
    PTRACE("                      meExternSramSupport = %d\n", info ->meExternSramSupport); \
    PTRACE("                                  busType = %d\n", info ->busType); \
    PTRACE("                                 busWidth = %d\n", info ->busWidth); \
    PTRACE("                          maxAXIAlignment = %d\n", info ->maxAXIAlignment); \
    PTRACE("                axi_burst_align_wr_common = %d\n", info ->axi_burst_align_wr_common); \
    PTRACE("                axi_burst_align_wr_stream = %d\n", info ->axi_burst_align_wr_stream); \
    PTRACE("            axi_burst_align_wr_chroma_ref = %d\n", info ->axi_burst_align_wr_chroma_ref); \
    PTRACE("              axi_burst_align_wr_luma_ref = %d\n", info ->axi_burst_align_wr_luma_ref); \
    PTRACE("                axi_burst_align_rd_common = %d\n", info ->axi_burst_align_rd_common); \
    PTRACE("                   axi_burst_align_rd_prp = %d\n", info ->axi_burst_align_rd_prp); \
    PTRACE("       axi_burst_align_rd_ch_ref_prefetch = %d\n", info ->axi_burst_align_rd_ch_ref_prefetch); \
    PTRACE("       axi_burst_align_rd_lu_ref_prefetch = %d\n", info ->axi_burst_align_rd_lu_ref_prefetch); \
    PTRACE("                axi_burst_align_wr_cuinfo = %d\n", info ->axi_burst_align_wr_cuinfo); \
    PTRACE("                            lowLatClkGate = %d\n", info ->lowLatClkGate); \
    PTRACE("                        h264NalRefIdc2bit = %d\n", info ->h264NalRefIdc2bit); \
    PTRACE("                         av1ExtensionFlag = %d\n", info ->av1ExtensionFlag); \
    PTRACE("                          roiAbsQpSupport = %d\n", info ->roiAbsQpSupport); \
    PTRACE("                              ROI8Support = %d\n", info ->ROI8Support); \
    PTRACE("                          h264CavlcEnable = %d\n", info ->h264CavlcEnable); \
    PTRACE("                         refRingBufEnable = %d\n", info ->refRingBufEnable); \
    PTRACE("                      disableRecWtSupport = %d\n", info ->disableRecWtSupport); \
    PTRACE("                      forceIntraCuSizeOpt = %d\n", info ->forceIntraCuSizeOpt); \
    PTRACE("                     backgroundDetSupport = %d\n", info ->backgroundDetSupport); \
    PTRACE("                           P010RefSupport = %d\n", info ->P010RefSupport); \
    PTRACE("                        streamBufferChain = %d\n", info ->streamBufferChain); \
    PTRACE("                               gmvSupport = %d\n", info ->gmvSupport); \
    PTRACE("                             IPCM8Support = %d\n", info ->IPCM8Support); \
    PTRACE("                av1InterpFilterSwitchable = %d\n", info ->av1InterpFilterSwitchable); \
    PTRACE("                               IframeOnly = %d\n", info ->IframeOnly); \
    PTRACE("                        dynamicRdoSupport = %d\n", info ->dynamicRdoSupport); \
    PTRACE("                     tuneToolsSet2Support = %d\n", info ->tuneToolsSet2Support); \
    PTRACE("                   hevcTemporalMvpSupport = %d\n", info ->hevcTemporalMvpSupport); \
    PTRACE("                    av1TemporalMvpSupport = %d\n", info ->av1TemporalMvpSupport); \
    PTRACE("                            tmvpMcSupport = %d\n", info ->tmvpMcSupport); \
    PTRACE("                        intraReconSupport = %d\n", info ->intraReconSupport); \
    PTRACE("                     av1IntraReconSupport = %d\n", info ->av1IntraReconSupport); \
    PTRACE("                         hevcCuRdoSupport = %d\n", info ->hevcCuRdoSupport); \
    PTRACE("                          av1CuRdoSupport = %d\n", info ->av1CuRdoSupport); \
    PTRACE("                        meqnSimpleBinCost = %d\n", info ->meqnSimpleBinCost); \
    PTRACE("                       streamMultiSegment = %d\n", info ->streamMultiSegment); \
    PTRACE("                            modSubjPrefer = %d\n", info ->modSubjPrefer); \
    PTRACE("                       twoRefPTMVPSupport = %d\n", info ->twoRefPTMVPSupport); \
  } while(0)

#ifdef __cplusplus
}
#endif

#endif  /* EWL_HWCFG_H */


/**
 * \defgroup ewl_info Core Information
 *
 * @{
 */

/** \brief The total register number of an encoder core. */
#define ASIC_SWREG_AMOUNT 512

/* HW ID check. */
/** \brief The bit mask to obtain the product ID from the hardware ID. */
#define HW_ID_PRODUCT_MASK 0xFFFF0000
/** \brief The bit mask to obtain the major version number from the hardware ID. */
#define HW_ID_MAJOR_NUMBER_MASK 0x0000FF00
/** \brief The bit mask to obtain the minor version number from the hardware ID. */
#define HW_ID_MINOR_NUMBER_MASK 0x000000FF
/** \brief The product ID derived from the hardware ID. */
#define HW_ID_PRODUCT(x) (((x & HW_ID_PRODUCT_MASK) >> 16))
/** \brief The major version number derived from the hardware ID. */
#define HW_ID_MAJOR_NUMBER(x) (((x & HW_ID_MAJOR_NUMBER_MASK) >> 8))
/** \brief The minor version number derived from the hardware ID. */
#define HW_ID_MINOR_NUMBER(x) ((x & HW_ID_MINOR_NUMBER_MASK))

/** \brief The hardware engine is H2, which is the first generation video encoder. */
#define HW_ID_PRODUCT_H2 0x4832
/** \brief The hardware engine is VC8000E.*/
#define HW_ID_PRODUCT_VC8000E 0x8000
/** \brief The hardware engine is VC9000E.*/
#define HW_ID_PRODUCT_VC9000E 0x9000
/** \brief The hardware engine is VC9000LE. */
#define HW_ID_PRODUCT_VC9000LE 0x9010
/** \brief The hardware engine is VC9800E. */
#define HW_ID_PRODUCT_VC9800E 0x9800

/** \brief Specifies whether the product ID is H2 series. */
#define HW_PRODUCT_H2(x) (HW_ID_PRODUCT_H2 == HW_ID_PRODUCT(x))
/** \brief Specifies whether the product ID is VC8000E. */
#define HW_PRODUCT_VC8000E(x) (HW_ID_PRODUCT_VC8000E == HW_ID_PRODUCT(x))
//0x80006000
/** \brief Specifies whether the product ID is VC8000E version 6.0. For details, see Section <i>Macro Definition</i>.*/
#define HW_PRODUCT_SYSTEM60(x)                    \
  ((HW_ID_PRODUCT_VC8000E == HW_ID_PRODUCT(x)) && \
   (HW_ID_MAJOR_NUMBER(x) == 0x60))
//0x80006010
/** \brief Specifies whether the product ID is VC8000E version 6.1. */
#define HW_PRODUCT_SYSTEM6010(x) \
  (HW_PRODUCT_SYSTEM60(x) && (HW_ID_MINOR_NUMBER(x) == 0x10))
//0x90001000
/** \brief Specifies whether the product ID is VC9000E. */
#define HW_PRODUCT_VC9000(x)        \
  (HW_ID_PRODUCT_VC9000E == HW_ID_PRODUCT(x) || HW_ID_PRODUCT_VC9800E == HW_ID_PRODUCT(x))
//0x90101000
/** \brief Specifies whether the product ID is VC8000LE. */
#define HW_PRODUCT_VC9000LE(x) (HW_ID_PRODUCT_VC9000LE == HW_ID_PRODUCT(x))

/** \brief The ASIC_ID, which indicates when the hardware can support BUILD_ID. */
#define MIN_ASIC_ID_WITH_BUILD_ID 0x80009100

/** \brief The register offset of the build ID register. */
#define HWIF_REG_BUILD_ID (509)
/** \brief The register offset of the build revision register. */
#define HWIF_REG_BUILD_REV (510)
/** \brief The register offset of the build date register. */
#define HWIF_REG_BUILD_DATE (511)

/** \brief  The hardware register configuration_1. */
#define HWIF_REG_CFG1 (80)
/** \brief The hardware register configuration_2. */
#define HWIF_REG_CFG2 (214)
/** \brief  The hardware register configuration_3.*/
#define HWIF_REG_CFG3 (226)
/** \brief The hardware register configuration_4. */
#define HWIF_REG_CFG4 (287)
/** \brief The hardware register configuration_AXI. */
#define HWIF_REG_CFGAXI (319)
/** \brief The hardware register configuration_5. */
#define HWIF_REG_CFG5 (430)
/** \brief The number of hardware configuration registers. */
#define HWIF_CFG_NUM 7

/** \brief The bit position that specifies core mode in the core information words. */
#define CORE_INFO_MODE_OFFSET 31
/** \brief The start bit position that specifies the core amount in the core information words. */
#define CORE_INFO_AMOUNT_OFFSET 28
/** \brief The maximum number of encoder cores that the software can support. */
#define MAX_SUPPORT_CORE_NUM 2//4//FIXME@RAMOPT

/* HW status register bits */
/** \brief The bit mask for the stream segment ready flag. */
#define ASIC_STATUS_SEGMENT_READY 0x1000
/** \brief The bit mask for the fuse error flag. */
#define ASIC_STATUS_FUSE_ERROR 0x200
/** \brief The bit mask for the slice ready flag. */
#define ASIC_STATUS_SLICE_READY 0x100
/** \brief The bit mask for the flag, which indicates whether a line buffer is used. */
#define ASIC_STATUS_LINE_BUFFER_DONE 0x080

/** \brief The bit mask for the flag, which indicates whether the slice information is updated in time. */
#define ASIC_STATUS_POLL_SLICEINFO_TIMEOUT 0x2000

/** \brief The bit mask for the external frame buffer compressor (UFBC) status flag, which
 * indicates whether an decode error occurs when UFBC is used to fetch the input picture data. */
#define ASIC_STATUS_UFBC_DEC_ERR 0x4000  //bit14
/** \brief The bit mask for the external frame buffer compressor (UFBC) status flag, which
 * indicates whether an config error occurs when UFBC is used to fetch the input picture data. */
#define ASIC_STATUS_UFBC_CFG_ERR 0x8000  //bit15

/** \brief The bit mask for the hardware timeout flag. */
#define ASIC_STATUS_HW_TIMEOUT 0x040

/** \brief The bit mask for the flag, which indicates the stream buffer is full. */
#define ASIC_STATUS_BUFF_FULL 0x020
/** \brief The bit mask for the flag, which indicates the hardware reset is completed. */
#define ASIC_STATUS_HW_RESET 0x010
/** \brief The bit mask for the bus error flag. */
#define ASIC_STATUS_ERROR 0x008
/** \brief The bit mask for the flag, which indicates whether the encoding of one frame is completed. */
#define ASIC_STATUS_FRAME_READY 0x004
/** \brief The bit mask for the flag, which indicates whether IRQ occurs. */
#define ASIC_IRQ_LINE 0x001

/** \brief The bit mask for valid status flags. For details, see Section <i>Macro Definition</i>. */
#define ASIC_STATUS_ALL                                                    \
  (ASIC_STATUS_SEGMENT_READY | ASIC_STATUS_FUSE_ERROR                     \
   | ASIC_STATUS_SLICE_READY | ASIC_STATUS_LINE_BUFFER_DONE                  \
   | ASIC_STATUS_HW_TIMEOUT | ASIC_STATUS_BUFF_FULL | ASIC_STATUS_HW_RESET  \
   | ASIC_STATUS_ERROR | ASIC_STATUS_FRAME_READY | ASIC_STATUS_UFBC_DEC_ERR \
   | ASIC_STATUS_UFBC_CFG_ERR | ASIC_STATUS_POLL_SLICEINFO_TIMEOUT)

/* Return values */
/** \brief API runs successfully. */
#define EWL_OK 0
/** \brief Errors occur when executing the function, such as the specified feature is not supported. */
#define EWL_ERROR -1
/** \brief The function execution fails, because the specified feature is not supported. */
#define EWL_NOT_SUPPORT -2

/** \brief The hardware is enabled properly even if an error is detected. */
#define EWL_HW_WAIT_OK EWL_OK
/** \brief An error is detected by the software. */
#define EWL_HW_WAIT_ERROR EWL_ERROR
/** \brief The hardware times out. */
#define EWL_HW_WAIT_TIMEOUT 1

/* Hardware configuration values */
/** \brief The bus type is unset. */
#define EWL_HW_BUS_TYPE_UNKNOWN 0
/** \brief The bus type is AHB. */
#define EWL_HW_BUS_TYPE_AHB 1
/** \brief The bus type is OCP.*/
#define EWL_HW_BUS_TYPE_OCP 2
/** \brief The bus type is AXI. */
#define EWL_HW_BUS_TYPE_AXI 3
/** \brief The bus type is PCI. */
#define EWL_HW_BUS_TYPE_PCI 4

/** \brief The bus width is unset. */
#define EWL_HW_BUS_WIDTH_UNKNOWN 0
/** \brief The bus width is 32 bits.*/
#define EWL_HW_BUS_WIDTH_32BITS 1
/** \brief The bus width is 64 bits. */
#define EWL_HW_BUS_WIDTH_64BITS 2
/** \brief The bus width is 128 bits. */
#define EWL_HW_BUS_WIDTH_128BITS 3

/** \brief The synthesis language is unset. */
#define EWL_HW_SYNTHESIS_LANGUAGE_UNKNOWN 0
/** \brief The synthesis language is VHDL. */
#define EWL_HW_SYNTHESIS_LANGUAGE_VHDL 1
/** \brief The synthesis language is VeriLog. */
#define EWL_HW_SYNTHESIS_LANGUAGE_VERILOG 2

/** \brief The current hardware configuration does not support the specified feature. */
#define EWL_HW_CONFIG_NOT_SUPPORTED 0
/** \brief The current hardware configuration supports the specified feature. */
#define EWL_HW_CONFIG_ENABLED 1

#ifndef VCECFG_MAX_CORE_NUM
/** \brief The supported maximum number of cores. */
#define EWL_MAX_CORES 4
#else
/** \brief The supported maximum number of cores. */
#define EWL_MAX_CORES VCECFG_MAX_CORE_NUM
#endif

/** \brief Obtains the core index from the core information words.
 * \n core_id[0:7]: The core index. */
/* get core slice index from core ID */
#ifndef CORE
#define CORE(id) ((u32)(id)&0xff)
#endif
/** \brief Obtains the slice node index from the core information words.
 * \n core_id[16:23]: The slice node index. */
#ifndef NODE
#define NODE(id) ((u32)(id) >> 16)
#endif
/** \brief The derived core ID, which is the combination of the slice node index and the core index. */
#ifndef COREID
/* combine slice node and core index as COREID */
#define COREID(node, core) (((u32)(node) << 16) | ((u32)(core)&0xff))
#endif
/**@}*/

#ifdef WIN32
#define GETPID() _getpid()
#else
#define GETPID() getpid()
#endif

/**
 * \defgroup ewl_mem Memory Management
 *
 * @{
 */

#define MEM_CHUNKS 720 /**< \brief The maximum number of linear memory used for status only. */
/** \brief The page size and alignment for linear memory chunks. */
#define LINMEM_ALIGN 4096

/** \brief The nearest aligned linear memory address before the input address x. */
#define NEXT_ALIGNED(x) \
  ((((ptr_t)(x)) + LINMEM_ALIGN - 1) / LINMEM_ALIGN * LINMEM_ALIGN)
/** \brief The nearest address aligned with "align" before the input address x. */
#define NEXT_ALIGNED_SYS(x, align) ((((ptr_t)(x)) + align - 1) / align * align)
 /** \brief The memory accessed by CPU. It is a non-secure memory. */
#define EWL_MEM_TYPE_CPU 0x0000U
#define EWL_MEM_TYPE_SLICE 0x0001U /**< \brief The output buffer for the encoder. */
#define EWL_MEM_TYPE_DPB 0x0002U   /**< \brief The input buffer or reconstructed frame buffer for the encoder. */
/** \brief The working buffer for the encoder. It is a non-secure memory. */
#define EWL_MEM_TYPE_VPU_WORKING 0x0003U
#define EWL_MEM_TYPE_VPU_WORKING_SPECIAL 0x0004U /**< \brief VPU reads the memory; CPU reads and writes the memory. */
#define EWL_MEM_TYPE_VPU_ONLY 0x0005U            /**< \brief VPU reads and writes the memory. */

/** \brief The bit mask (bits [22:16]) that indicates the usage hint of the buffer. */
#define EWL_MEM_USAGE_HINT_MASK 0x7F0000U
/** \brief The least significant bit (LSB) of memory usage hint bits in memory attribute words. */
#define EWL_MEM_USAGE_HINT_SHIFT 16
/** \brief Bit 23, which indicates whether the memory attribute is META or CORE. */
#define EWL_MEM_ATTR_SHIFT 23
/** The security attribute of a buffer.*/
enum MEM_ATTR {
  META, /**< The buffer does not have sensitive data. */
  CORE  /**< The buffer has sensitive data. */
};
/** \brief Non-specified attributes of a memory. These attribute are regarded as META. */
#define ATTR_NOT_DEF META

/**@} */

#define EWL_HINT_LIST(DEF) \
  /** (Default) The attribute is not defined. */ \
  DEF(EWL_MEM_USAGE_UNDEFINED, ATTR_NOT_DEF) /* invalid */ \
  /** The attribute for an input picture buffer. */ \
  DEF(EWL_MEM_USAGE_IN_SURFACE, CORE) /* R1, */ \
  /** The attribute for a QP map buffer. */ \
  DEF(EWL_MEM_USAGE_IN_QPMAP, META)  /* R7, W64+ */  \
  /** The attribute for a buffer that stores overlay pictures. */ \
  DEF(EWL_MEM_USAGE_IN_OVERLAY, ATTR_NOT_DEF) /* R11 */ \
  /** The attribute for a CU control buffer. */ \
  DEF(EWL_MEM_USAGE_IN_CUCTLMAP, META) /* R7 */ \
  /** (Reserved) The attribute for a CU index map buffer. */ \
  DEF(EWL_MEM_USAGE_IN_CUIDXMAP, ATTR_NOT_DEF) /* test */ \
  /** The attribute for downscale input for pass-1 encoding when lookahead is enabled. */ \
  DEF(EWL_MEM_USAGE_IN_DOWNSACLE, CORE) /* R1 */ \
  /** The attribute for DEC400 tile status of input picture buffer. */ \
  DEF(EWL_MEM_USAGE_IN_SURFACE_DECTS, META) /* R31 */ \
  /** The attribute for DEC400 tile status of input overlay buffer. */ \
  DEF(EWL_MEM_USAGE_IN_OVERLAY_DECTS, META) /* R31 */ \
  /** (Reserved) The attribute for MV information. */ \
  DEF(EWL_MEM_USAGE_IN_MVINFO, ATTR_NOT_DEF) /* dev */ \
  /** (For test only) The attribute for input picture translation. */\
  DEF(EWL_MEM_USAGE_IN_TRANSFORM, ATTR_NOT_DEF) /* test */ \
  /** The attribute for a line buffer (as input picture) in low-latency mode. */ \
  DEF(EWL_MEM_USAGE_IN_LINEBUF, CORE) /* R1 */ \
  /** The attribute for OSD map buffer. */ \
  DEF(EWL_MEM_USAGE_IN_OSDMAP, ATTR_NOT_DEF) \
  /** The attribute for output stream. */ \
  DEF(EWL_MEM_USAGE_OUT_STRM, CORE) /* W4 */ \
  /** The attribute for output CU information.  */ \
  DEF(EWL_MEM_USAGE_OUT_CUINFO, META) /* W2 */ \
  /** The attribute for the buffer that stores the NAL unit size of output streams. */ \
  DEF(EWL_MEM_USAGE_OUT_SIZETABLE, META) /* W3 */ \
  /** The attribute for status buffer which have bits count of every CTB. */ \
  DEF(EWL_MEM_USAGE_OUT_CTBBITS, ATTR_NOT_DEF) /* dev */ \
  /** The attribute for downscale output picture. */ \
  DEF(EWL_MEM_USAGE_OUT_SCAL, CORE) /* W8 */ \
  /** The attribute for reference compression tables. */ \
  DEF(EWL_MEM_USAGE_TMP_RFCTS, CORE) /* R3, R4, R5, R6, R8, W5, W6, W7 */ \
  /** The attribute for scratch of an entropy engine. */ \
  DEF(EWL_MEM_USAGE_TMP_COEFF, META) /* R2, W2 */ \
  /** The attribute for scratch of tile context. */ \
  DEF(EWL_MEM_USAGE_TMP_TILECTX, ATTR_NOT_DEF) /* not configure */ \
  /** The attribute for scratch of tile edge. */ \
  DEF(EWL_MEM_USAGE_TMP_TILEHEIGHT, ATTR_NOT_DEF) /* not configure */ \
  /** The attribute for a buffer that stores reconstructed frames. */ \
  DEF(EWL_MEM_USAGE_TMP_RECON, CORE) /* R3, R4, R5, R6, R8, W5, W6, W7 */ \
  /** The attribute for a buffer used for CTBRC. */ \
  DEF(EWL_MEM_USAGE_TMP_CTBRC, META) /* R9, W9*/ \
  /** The attribute for multi-core synchronization words. */ \
  DEF(EWL_MEM_USAGE_TMP_MCSYNC, META) /* R10 R14, W10, W13*/ \
  /** The attribute for a TMVP buffer. */ \
  DEF(EWL_MEM_USAGE_TMP_TMVP, META) /* R15, W3 */ \
  /** The attribute for a cost buffer. */ \
  DEF(EWL_MEM_USAGE_TMP_COST, META) /* R64+, W64+ */ \
  /** The attribute for the external SRAM of ME. */ \
  DEF(EWL_MEM_USAGE_TMP_EXTSRAM, CORE) /* not configure */ \
  /** The attribute for colocate information for H264. */ \
  DEF(EWL_MEM_USAGE_TMP_H264COL, META) /* R2, W1 */ \
  /** The attribute for frame context for AV1. */ \
  DEF(EWL_MEM_USAGE_TMP_AV1FRMCTX, META) /* R13,W12 */ \
  /** The attribute for pre-carry buffer for AV1. */ \
  DEF(EWL_MEM_USAGE_TMP_AV1PRC, CORE) /* R12, W4 */ \
  /** The attribute for frame context for VP9. */ \
  DEF(EWL_MEM_USAGE_TMP_VP9FRMCTX, META) /* R13,W12 */ \
  /** The attribute for frame status output. */ \
  DEF(EWL_MEM_USAGE_TMP_FRAMEINFO, ATTR_NOT_DEF) /* not configure */ \
  /** The attribute for the slice information used in low-latency encoding DDR mode. */ \
  DEF(EWL_MEM_USAGE_TMP_SLICEINFO, META) /* R11 */ \
  DEF(EWL_MEM_USAGE_MAX, ATTR_NOT_DEF)    /* invalid */

#define DEF_ENUM(enum, attr) enum,

/**
 * \addtogroup ewl_mem
 *
 * @{
 */
/** Specifies the usage of a buffer. */
enum EWLMemUsageHint {
  EWL_HINT_LIST(DEF_ENUM)
};

/**@} */

#define DEF_ATTR(enum, attr) enum##_ATTR=attr,

/** Specifies the buffer attributes when the security mode is enabled. */
enum EWLMemAttribute {
  EWL_HINT_LIST(DEF_ATTR)
};

#define USAGE_ATTR(usage) usage##_ATTR

/**
 * \addtogroup ewl_mem Memory Management
 *
 * @{
 */
/** \brief Sets the memory usage hint and security flag in memory attribute words. For details, see Section <i>Macro Definition</i>.
 *
 * \param [inout] mem_type Input: The memory attribute words and other attributes. <br>
 *    Output: The input data added with usage hint and security flag.
 * \param [in] usage The memory usage hint.
 * \param [in] is_secure The security status of the memory.
 */
#define SET_MEM_USAGE(mem_type, usage, is_secure) do { \
      (mem_type) |= ((usage)<<EWL_MEM_USAGE_HINT_SHIFT | \
      ((USAGE_ATTR(usage)&is_secure)<<EWL_MEM_ATTR_SHIFT)); \
  } while(0)

/** \brief Specifies whether the memory is secure by parsing the memory attribute flag. */
#define EWLGetMemAttribute(mem_type) ((mem_type>>EWL_MEM_ATTR_SHIFT)&0x1)

#define CPU_RD 0x0100U /**< \brief CPU reads the memory.*/
#define CPU_WR 0x0200U /**< \brief CPU writes the memory.*/
#define VPU_RD 0x0400U /**< \brief VPU reads the  memory.*/
#define VPU_WR 0x0800U /**< \brief VPU writes the memory.*/
#define EXT_RD 0x1000U /**< \brief External IP reads the memory.*/
#define EXT_WR 0x2000U /**< \brief External IP writes the memory.*/

 /** \brief The provided parameter is wrong. */
#define EWL_PAR_ERROR EWL_ERROR
 /** \brief Errors occur, when the specified function runs. */
#define EWL_HW_ERROR -2

/** Specifies the data transfer direction of EWL. */
enum EWLMemSyncDirection {
  HOST_TO_DEVICE = 0, /**< Copies data from the host memory to the device memory. */
  DEVICE_TO_HOST = 1  /**< Copies data from the device memory to the host memory. */
};

#define UFBC_HW_ID_1_1_0 0x42431000
#define UFBC_HW_ID_1_1_1 0x42431100

/** \brief Specifies allocated memory information for counting. */
typedef struct EWLMemoryStatistics {
  u32 maxHeapMemory;      /**< \brief The maximum memory value. */
  u32 memoryValue;        /**< \brief Memory value. */
  u32 maxHeapMallocTimes; /**< \brief The number of times <tt>EWLmalloc()</tt> or <tt>EWLcalloc()</tt> are called. */
  u32 averageHeapMallocTimes; /**< \brief The maximum heapMallocTimes. \n For example, if <tt>EWLmalloc()</tt>
  is called 10 times and <tt>EWLfree()</tt> is called twice, <tt>averageHeapMallocTimes</tt> and <tt>heapMallocTimes</tt> are eight.
  If then, <tt>EWLfree()</tt> is called one more time, <tt>heapMallocTimes</tt> is seven, but <tt>averageHeapMallocTimes</tt>
  is still eight. On the contrary, if <tt>EWLmalloc()</tt> is called one more time, <tt>HeapMallocTimes</tt> becomes nine,
  larger than the former eight, and <tt>maxHeapMallocTimes</tt> becomes nine as well. */
  u32 heapMallocTimes; /**< \brief The number of times [<tt>EWLmalloc()</tt> - <tt>EWLfree()</tt>] are called. */

  u32 maxLinearMemory;   /**< \brief The maximum memory value of a linear buffer. */
  u32 linearMemoryValue; /**< \brief The memory value of linear buffer. */
  /** \brief The number of linear buffers allocated. The number is counted by how many times
   * <tt>EWLMallocLinear()</tt> or <tt>EWLMallocRefFrm()</tt> are called. */
  u32 maxLinearMallocTimes;
  /** \brief The maximum <tt>linearMallocTimes</tt>. \n For example, if <tt>EWLMallocLinear()</tt> is called ten times,
   * and <tt>EWLFreeLinear()</tt> is called twice, <tt>averageLinearMallocTimes</tt> and <tt>linearMallocTimes</tt> are eight.
   * If then, <tt>EWLFreeLinear()</tt> is called one more time, <tt>linearMallocTimes</tt> is seven,
   * but <tt>maxLinearMallocTimes</tt> is still eight. On the contrary, if <tt>EWLMallocLinear()</tt> is called one more time,
   * <tt>linearMallocTimes</tt> becomes nine, larger than the former eight, and <tt>maxLinearMallocTimes</tt> becomes nine as well. */
  u32 averageLinearMallocTimes;
  /** \brief The number of linear buffers that are not released when an instance is released.
   * This number is counted by subtracting the number of times <tt>EWLMallocLinear()</tt> is called from
   * the number of times <tt>EWLFreeLinear()</tt> is called.*/
  u32 linearMallocTimes;
} EWLMemoryStatistics_t;

/** \brief The information of the memory allocated as a linear buffer. */
typedef struct EWLLinearMem {
  u32 *virtualAddress;   /**< \brief The aligned virtual address of the memory for CPU to access. */
  ptr_t busAddress;      /**< \brief The aligned bus address of the memory for VPU to access. */
  u32 size;              /**< \brief The size of memory, in bytes. */
  u32 *allocVirtualAddr; /**< \brief The original virtual address of the memory for CPU to access. */
  ptr_t allocBusAddr;    /**< \brief The original bus address of the memory for VPU to access. */
  unsigned long id;      /**< \brief The ID of the buffer. */
  u32 mem_type;          /**< \brief The owner and attributes of the memory.
                          * \n Bit 23 indicates whether the buffer has sensitive data. See @ref MEM_ATTR.
                          * \n Bits [22:16] indicate the usage hints of memory. See @ref EWLMemUsageHint.
                          * \n Bits [15:8] indicate which IP is allowed to access the memory.
                          * \n Bits [7:0] indicate the attributes of the memory.*/
  u32 total_size;        /**< \brief The size of the allocated buffer, in bytes. */
  void *priv;            /**< \brief Customer-defined field for application use. */
} EWLLinearMem_t;

/**@} */

#ifdef MEM_ONLY_DEV_CHECK
 /** \brief Specifies whether the bus address of a linear memory is set. */
#define EWL_DEVMEM_VAILD(mem) ((mem).busAddress != 0)
 /** \brief reset the bus address of a linear memory as zero. */
#define EWL_CLEAN_DEVMEM_ADDR(mem) ((mem).busAddress = 0)
#else
 /** \brief Specifies whether the virtual address of a linear memory is set. */
#define EWL_DEVMEM_VAILD(mem) ((mem).virtualAddress != NULL)
 /** \brief reset the virtual address of a linear memory as NULL. */
#define EWL_CLEAN_DEVMEM_ADDR(mem) ((mem).virtualAddress = NULL)
#endif

/**
 * \defgroup ewl_api EWL API
 *
 * @{
 */

/** \brief The parameters required for EWL initialization. */
typedef struct EWLInitParam {
  /** \brief The encoding format. See @ref CLIENT_TYPE. */
  u32 clientType;
  /** \brief The context used for initialization. The context is sent to the application by EWL. */
  void *context;
  /** \brief The index of the slice node for multi-node VPU. \n This field is valid only for specific versions. */
  u32 slice_idx;
  /** \brief Specifies whether the hardware supports MMU. \n 0: Does not support. \n 1: Supports. */
  u32 mmuEnable;
  /** \brief The device name of the enc driver. */
  char *enc_dev;
  /** \brief The device name of the memalloc driver. */
  char *mem_dev;
  /** \brief Specifies whether to use VCMD. \n 0: Not use. \n 1: Use. */
  i32 useVcmd;
} EWLInitParam_t;

/** \brief The L2Cache. */
typedef struct CacheData {
  void **cache; /**< \brief The L2Cache. */
} CacheData_t;

typedef void (*CoreWaitCallBackFunc)(const void *ewl, void *data);

/** \brief The information of a job  */
typedef struct EWLCoreWaitJob {
  struct node *next; /**< \brief The next job. */
  u32 id;            /**< \brief The ID of the job the structure describes. */
  u32 core_id;       /**< \brief The index of the hardware subsystem. */
  const void *inst;  /**< \brief The <tt>vcenc_instance</tt> structure. */
  /** \brief All hardware registers. <tt>ASIC_SWREG_AMOUNT</tt> indicates the maximum number of hardware registers. */
  u32 VCE_reg[ASIC_SWREG_AMOUNT];
  i32 out_status; /**< \brief The interrupt status of the job. \n The value can be:
                               \n - @ref ASIC_STATUS_SEGMENT_READY
                               \n - @ref ASIC_STATUS_FUSE_ERROR
                               \n - @ref ASIC_STATUS_SLICE_READY
                               \n - @ref ASIC_STATUS_LINE_BUFFER_DONE
                               \n - @ref ASIC_STATUS_HW_TIMEOUT
                               \n - @ref ASIC_STATUS_BUFF_FULL
                               \n - @ref ASIC_STATUS_HW_RESET
                               \n - @ref ASIC_STATUS_ERROR
                               \n - @ref ASIC_STATUS_FRAME_READY */
  i32 out_poll_sliceinfo_status; /**< \brief The status indicating whether timeout occurs when polling sliceinfo. */
  i32 out_ufbc_status; /**< \brief the interrupt status of ufbc, mainly for ASIC_STATUS_UFBC_DEC_ERR */
  u32 slices_rdy;     /**< \brief The number of completed slices. */
  u32 low_latency_rd; /**< \brief The number of CTB rows that the encoder fetches from the input buffer for the job. */
  u32 dec400_enable;  /**< \brief The status of DEC400. \n <tt>1</tt>: Bypassed.  \n <tt>2</tt>: Enabled. */
  //VCDec400data dec400_data;
  /** \brief (Reserved) The DEC400 callback function. */
  CoreWaitCallBackFunc dec400_callback;
  /** \brief Pointer to the DEC400 data. */
  void *dec400_data;
  u32 axife_enable; /**< \brief The status of AXI_FE. \n <tt>0</tt>: Disabled.  \n <tt>1</tt>: Enabled.  \n <tt>2</tt>: Bypassed.  \n <tt>3</tt>: Security mode.*/
  /** \brief The AXI_FE callback function. */
  CoreWaitCallBackFunc axife_callback;
  u32 l2cache_enable; /**< \brief The status of L2Cache. \n <tt>0</tt>: Disabled. \n <tt>1</tt>: Enabled. */
  CacheData_t l2cache_data; /**< \brief The L2Cache. */
  /** \brief The L2Cache callback function. */
  CoreWaitCallBackFunc l2cache_callback;
  /** \brief Whether to enable UFBC. \n
             <tt>0</tt>: Disable. \n
             <tt>1</tt>: Enabled. */
  u32 ufbcEnable;
  /** \brief The UFBC callback. See <tt>EncUfbcAsicStop</tt>. */
  CoreWaitCallBackFunc ufbc_callback;
} EWLCoreWaitJob_t;

/** \brief Parameters for jobs to be processed in a thread. */
typedef struct EWLCoreWait {
  struct queue jobs;         /**< \brief The queue of unfinished jobs, including jobs to be processed and jobs under processing. */
  vsios_mutex_t job_mutex; /**< \brief The mutex for protecting job_pool. */
  vsios_sem_t job_cond;    /**< \brief The condition variable for job_mutex (implemented as semaphore). */

  /** \brief The queue of all completed jobs, including abnormal and normal ones. */
  struct queue out;
  vsios_mutex_t out_mutex; /**< \brief The mutex for protecting the out queue. */
  vsios_sem_t out_cond;    /**< \brief The condition variable for out_mutex (implemented as semaphore). */
  vsios_thread_t thread; /**< \brief The thread. */
  bool bFlush;               /**< \brief Whether the last job is queued. \n <tt>0</tt>: No.  \n <tt>1</tt>: Yes. */

  /** \brief The reference frame counter for encoding types of HEVC, H264, AV1, and VP9.
   * \n When the EWL instance is released, the field value equals 0. */
  u32 refer_counter;
  /** \brief The pool that stores jobs. */
  struct queue job_pool;
} EWLCoreWait_t;

/** \brief The job configuration. */
typedef struct EWLWaitJobCfg {
  u32 waitCoreJobid; /**< \brief The job ID maintained by the kernel driver. */
  u32 dec400_enable; /**< \brief The status of DEC400. \n <tt>1</tt>: Bypassed. \n <tt>2</tt>: Enabled. */
  void *dec400_data; /**< \brief The DEC400. */
  CoreWaitCallBackFunc dec400_callback; /**< \brief dec400 callback resvered for furture */
  /** \brief The status of AXI_FE. \n
             <tt>0</tt>: Disabled. \n
             <tt>1</tt>: Enabled. \n
             <tt>2</tt>: Bypassed. \n
             <tt>3</tt>: Security mode. */
  u32 axife_enable;
  /** \brief The AXI_FE callback function. */
  CoreWaitCallBackFunc  axife_callback;
  u32 l2cache_enable; /**< \brief The status of L2Cache. \n <tt>0</tt>: Disabled. \n <tt>1</tt>: Enabled. */
  void *l2cache_data; /**< \brief The L2Cache. */
  /** \brief The L2Cache callback function. */
  CoreWaitCallBackFunc l2cache_callback;
  /** \brief The Status of UFBC. \n
             <tt>0</tt>: Disabled. \n
             <tt>1</tt>: Enabled. */
  u32 ufbcEnable;
  /** \brief The UFBC callback. See <tt>EncUfbcAsicStop</tt>. */
  CoreWaitCallBackFunc ufbc_callback;
} EWLWaitJobCfg_t;

/** \brief The information about a VCMD buffer, when reserving buffer resources. */
typedef struct {
  u16 size;         /**< \brief [in] The buffer size to reserve. */
  u16 id;           /**< \brief [out] The ID of the VCMD buffer. */
  ptr_t status_ba;  /**< \brief [out] The bus address of the status buffer. */
  u32 *cmdbuf_va;   /**< \brief [out] The virtual address of the VCMD buffer. */
  u32 core_mask;    /**< \brief [in] The mask that indicates which core is selected. */
  u32 priority;     /**< \brief [in] The priority of the selected VCMD buffer. */
} EWLResourceVcmdBuf;

/** \brief Information about reserved resources. */
typedef union EWLResource {
  /** \brief The information about the VCMD buffer, when reserving buffer resources. */
  EWLResourceVcmdBuf vcmdbuf;
} EWLResource_t;

/** The hardware engine type. */
typedef enum {
  /** Video or image encoder main core */
  EWL_CLIENT_TYPE_MAIN = 0U,
  /** H.264 encoder */
  EWL_CLIENT_TYPE_H264_ENC = 0U,
  /** HEVC encoder */
  EWL_CLIENT_TYPE_HEVC_ENC = 1U,
  /** VP9 encoder */
  EWL_CLIENT_TYPE_VP9_ENC = 2U,
  /** JPEG encoder */
  EWL_CLIENT_TYPE_JPEG_ENC = 3U,
  /** CuTree analyzer engine */
  EWL_CLIENT_TYPE_CUTREE = 4U,
  /** (Reserved) Video stabilization engine */
  EWL_CLIENT_TYPE_VIDEOSTAB = 5U,
  /** DEC400 engine */
  EWL_CLIENT_TYPE_DEC400 = 6U,
  /** AV1 encoder */
  EWL_CLIENT_TYPE_AV1_ENC = 7U,
  /** L2Cache engine */
  EWL_CLIENT_TYPE_L2CACHE = 8U,
  /** AXI_FE engine */
  EWL_CLIENT_TYPE_AXIFE = 9U,
  /** APB filter engine */
  EWL_CLIENT_TYPE_APBFT = 10U,
  /** AXI_FE_1 engine */
  EWL_CLIENT_TYPE_AXIFE_1 = 11U,
  /** memalloc */
  EWL_CLIENT_TYPE_MEM = 12U,
  /** MMU0 */
  EWL_CLIENT_TYPE_MMU0 = 13U,
  /** MMU1 */
  EWL_CLIENT_TYPE_MMU1 = 14U,
  /** UFBC */
  EWL_CLIENT_TYPE_UFBC = 15U,
  EWL_CLIENT_TYPE_MAX
} CLIENT_TYPE;

/** \brief Specifies whether the client type is HEVC */
#define EWL_IS_HEVC_CLIENT(clientType)  ((clientType) == EWL_CLIENT_TYPE_HEVC_ENC)
/** \brief Specifies whether the client type is H.264. */
#define EWL_IS_H264_CLIENT(clientType)  ((clientType) == EWL_CLIENT_TYPE_H264_ENC)
/** \brief Specifies whether the client type is AV1. */
#define EWL_IS_AV1_CLIENT(clientType)   ((clientType) == EWL_CLIENT_TYPE_AV1_ENC)
/** \brief Specifies whether the client type is VP9. */
#define EWL_IS_VP9_CLIENT(clientType)   ((clientType) == EWL_CLIENT_TYPE_VP9_ENC)
/** \brief Specifies whether the client type is JPEG. */
#define EWL_IS_JPEG_CLIENT(clientType)  ((clientType) == EWL_CLIENT_TYPE_JPEG_ENC)
/** \brief Specifies whether the client type is CuTree. */
#define EWL_IS_CUTREE(clientType)       ((clientType) == EWL_CLIENT_TYPE_CUTREE)
/** \brief Specifies whether the client type is video stabilization. */
#define EWL_IS_VIDEOSTAB(clientType)    ((clientType) == EWL_CLIENT_TYPE_VIDEOSTAB)
/** \brief Specifies whether the client is for video encoding. For details, see Section <i>Macro Definition</i>. */
#define EWL_IS_VIDEO_CLIENT(clientType) ( EWL_IS_HEVC_CLIENT(clientType) \
	    || EWL_IS_H264_CLIENT(clientType) \
        || EWL_IS_AV1_CLIENT(clientType) \
        || EWL_IS_VP9_CLIENT(clientType))
/** \brief Specifies whether the given client type is supported and enabled. For details, see Section <i>Macro Definition</i>. */
#define EWL_SUPPORT_CLIENT(cfg, clientType) \
        (((cfg->hevcEnabled == 1) && EWL_IS_HEVC_CLIENT(clientType)) \
        || ((cfg->h264Enabled == 1) && EWL_IS_H264_CLIENT(clientType)) \
        || ((cfg->av1Enabled == 1) && EWL_IS_AV1_CLIENT(clientType)) \
        || ((cfg->vp9Enabled == 1) && EWL_IS_VP9_CLIENT(clientType)) \
        || ((cfg->jpegEnabled == 1) && EWL_IS_JPEG_CLIENT(clientType)) \
        || ((cfg->vsSupport == 1) && EWL_IS_VIDEOSTAB(clientType)))
/** \brief Specifies whether the client is CuTree and whether it is supported. */
#define EWL_SUPPORT_CUTREE(cfg, clientType) \
        ((cfg->cuTreeSupport == 1) && EWL_IS_CUTREE(clientType))

extern u32 (*pollInputLineBufTestFunc)(void);

/*------------------------------------------------------------------------------
      4.  Function prototypes
  ------------------------------------------------------------------------------*/

/**@} */

/**
 * \addtogroup ewl_info
 *
 * @{
 */
/** Obtains the hardware ID of the specified core.
 * \param [in] core_id The ID to identify the core.
 * \param [in] ctx The context used to access the hardware, for example, the device description for this instance.
 * \return The hardware ID of the specified core.
 */
u32 EWLReadAsicID(u32 core_id, const void *ctx);

/** Obtains the core number.
 * \param [in] ctx The context used to access the hardware, for example, the device description for this instance.
 * \return The core number in the system.
 */
u32 EWLGetCoreNum(const void *ctx);

/** Specifies whether the CuTree hardware is available in the system.
 * \param [in] inst An EWL instance.
 * \return <tt>EWL_ERROR</tt>
 * \return <tt>EWL_OK</tt>
 */
i32 EWLCheckCutreeValid(const void *inst);

/** Obtains the DEC400 attribute for specified input pixel format.
 * \param [in] pixel_format The picture pixel format, which is compressed by DEC400, in the buffer.
 * \param [out] tile_size A pointer to the tile size of the corresponding pixel format.
 * \param [out] bits_tile_in_table A pointer to the bits used in the table for one tile.
 * \param [out] planar420_cbcr_table_style A pointer to the chroma table attribute.
 */
void EWLGetDec400Attribute(u32 *tile_size,
                           u32 *bits_tile_in_table,
                           u32 *planar420_cbcr_table_style);

/** Obtains hardware configuration information.
 * \param [in] core_id The ID to identify the core.
 * \param [in] ctx The context used to access the hardware, for example, the device description for this instance.
 * \return The hardware configuration of the specified core.
*/
const EWLHwConfig_t *EWLReadAsicConfig(u32 core_id, const void *ctx);

/**@} */

/**
 * \addtogroup ewl_api
 *
 * @{
 */
/** Obtains the core ID of a core, to which the DEC400 is attached.
 * \param [in] inst An EWL instance.
 * \return The index of the subsystem.
 */
i32 EWLGetDec400Coreid(const void *inst);

/** Obtains the VCMD hardware version ID.
 * \param [in] inst An EWL instance.
 * \return The version ID of the VCMD.
 */
u32 EWLGetVcmdVersionId(const void *inst);

/** Maps the registers for access from user space.
 * \param [in] ewl An EWL instance.
 * \return <tt>0</tt>: Mapping succeeds.
 * \return <tt>-1</tt>: Mapping fails.
 */
int MapAsicRegisters(void *ewl);

/** Obtains the client type used to initialize the instance.
 * \param [in] inst An EWL instance.
 * \return The client type.
 */
u32 EWLGetClientType(const void *inst);

/** Converts the client type to core type.
 * \param [in] client_type The client type to write.
 * \return The core type.
 */
u32 EWLGetCoreTypeByClientType(u32 client_type);

/** Initializes an EWL instance.
 * \param [in] param The configuration for this instance.
 * \return An EWL instance, or NULL in case of function failure.
 */
const void *EWLInit(EWLInitParam_t *param);

/** Releases an EWL instance.
   * \return <tt>EWL_OK</tt>
   * \return <tt>EWL_ERROR</tt>
   */
i32 EWLRelease(const void *inst);

/** Reserves the hardware resources for the current EWL instance.
 *
 * This function should be called after the software prepared a buffer and register values for encoding one frame and the hardware can start encoding.
 * It is blocked if resources are unavailable.
 * After the hardware resources are reserved, the software can access these resources directly.
 *
 * \param [inout] core_info The information to specify which core or cores can be used for the current encoding.
 * \param [out] job_id The ID used to note that the current frame encoding job is filled.
 * \return <tt>EWL_OK</tt>
 * \return <tt>EWL_ERROR</tt>
 */
i32 EWLReserveHw(const void *inst, u32 *core_info, u32 *job_id);

/** Releases the hardware resources.
 *
 * This function should be called after the hardware finishes encoding one frame and the software has
 * obtained the encoding information from hardware registers. After calling this function, the software
 * releases the hardware resources reserved for the current instance. Then, other instances can access
 * these hardware resources.
 *
 * The software can only access hardware resources, such as registers, between calling
 * <tt>EWLReserveHw()</tt> and <tt>EWLReleaseHw()</tt>.
 */
void EWLReleaseHw(const void *inst);

/** Obtains the number of hardware cycles used for encoding one frame.
 *
 * The cycles are counted according to the encoder clock.
 *
 * \return The number of hardware cycles used for encoding one frame.
 */
u32 EWLGetPerformance(const void *inst);

/**@} */

void EwlReleaseCoreWait(void *inst);

EWLCoreWaitJob_t *EWLDequeueCoreOutJob(const void *inst, u32 waitCoreJobid);

void EWLEnqueueOutToWait(const void *inst, EWLCoreWaitJob_t *job);

void EWLEnqueueWaitjob(const void *inst, EWLWaitJobCfg_t *cfg);

void EWLPutJobtoPool(const void *inst, struct node *job);

/**
 * \addtogroup ewl_mem
 *
 * @{
 */
/** Allocates a linear memory for reference frames. Reference frames are only accessed by hardware.
 * \param [in] size The bytes to be allocated.
 * \param [in] alignment The required alignment of the buffer.
 * \param [out] info The information of the memory allocated as a linear buffer.
 */
i32 EWLMallocRefFrm(const void *instance, u32 size, u32 alignment,
                    EWLLinearMem_t *info);

/** Releases the linear buffer allocated by EWLMallocRefFrm().
 *
 * \param [in] info The information to identify the buffer to be released.
 */
void EWLFreeRefFrm(const void *inst, EWLLinearMem_t *info);

/** Specifies software/hardware-shared buffer without memory synchronization.
 *
 * When MMU is disabled, this buffer should be physically continuous.
 *
 * \param [in] size The bytes to be allocated.
 * \param [in] alignment The required alignment of the buffer.
 * \param [out] info The information of the memory allocated as a linear buffer.
 */
i32 EWLMallocLinearExe(const void *instance, u32 size, u32 alignment,
                    EWLLinearMem_t *info);

//FIXME@DBG
#define EWLMallocLinear(a,b,c,d) EWLMallocLinearExe(a,b,c,d)
//#define EWLMallocLinear(a,b,c,d) (EWLMallocLinearExe(a,b,c,d) + bk_printf("%s:%d\n", __FILE__, __LINE__) / 10000)

/** Releases the linear memory allocated by <tt>EWLMallocLinear()</tt>.
 *
 * \param [in] info The information to identify the buffer to be released.
 */
void EWLFreeLinear(const void *inst, EWLLinearMem_t *info);

/**@} */

/* D-Cache coherence flush (obsolete) */ /* Not in use currently */
void EWLDCacheRangeFlush(const void *instance, EWLLinearMem_t *info);
/* D-Cache coherence refresh (obsolete) */ /* Not in use currently */
void EWLDCacheRangeRefresh(const void *instance, EWLLinearMem_t *info);

/**
 * \addtogroup ewl_api
 *
 * @{
 */
/**
 * Writes a value to a hardware register.
 *
 * All registers are written before the hardware is enabled for encoding.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 */
void EWLWriteReg(const void *inst, u32 offset, u32 val);

/** Writes back a value to a hardware register.
 *
 * This function is called when a callback is completed, or a frame encoding in a multi-core scenario is completed.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 */
void EWLWriteBackReg(const void *inst, u32 offset, u32 val);

/** Writes a value to a hardware register by the specified core ID.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 * \param [in] core_id The ID to specify the core to write.
 */
void EWLWriteCoreReg(const void *inst, u32 offset, u32 val, u32 core_id);

/** Writes a value to a hardware register by the specified client type.
 *
 *  This function is often used to access peripheral registers of sub-IPs.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 * \param [in] client_type The client type to write.
 */
void EWLWriteRegbyClientType(const void *inst, u32 offset, u32 val,
                             u32 client_type);

/** Writes back a value to a hardware register by the specified client type.
 *
 *  This function is often used to access peripheral registers of sub-IPs.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 * \param [in] client_type The client type to write.
 */
void EWLWriteBackRegbyClientType(const void *inst, u32 offset, u32 val,
                                 u32 client_type);

/** Obtains the value in a status register.
 *
 * Such registers are updated by the hardware and are read after an interrupt is generated.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \return The value of the hardware register.
 */
u32 EWLReadReg(const void *inst, u32 offset);

/** Obtains the value in a status register of one client.
 *
 * Such registers are updated by the hardware and are read after an interrupt is generated.
 *
 * This function is often used to access peripheral registers of sub-IPs.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \return The value of the hardware register.
 */
u32 EWLReadRegbyClientType(const void *inst, u32 offset, u32 client_type);

/* Writing all registers in one call */ /* Not in use currently */
void EWLWriteRegAll(const void *inst, const u32 *table, u32 size);
/* Reading all registers in one call */ /* Not in use currently */
void EWLReadRegAll(const void *inst, u32 *table, u32 size);

/** Enables the hardware to start encoding a frame.
 * \param [in] offset The offset related to the hardware register base address, in bytes. Its value should be set to 0x14.
 * \param [in] val The value to be written into the register. Bit 0 of this parameter should be 1.
 */
i32 EWLEnableHW(const void *inst, u32 offset, u32 val);

/** Disables the hardware and stops encoding.
 * \param [in] offset The offset related to the hardware register base address, in bytes. Its value should be set to 0x14.
 * \param [in] val The value to be written into the register. Bit 0 of this parameter should be 0.
 */
void EWLDisableHW(const void *inst, u32 offset, u32 val);

/**
 * Waits for the hardware to complete encoding.
 *
 * After the hardware is enabled, this function is called to wait for frame encoding completion or error interrupts.
 * In polling mode, the software polls the hardware status register to check whether there is any hardware status change.
 * In interrupt mode, the software is in "sleep" status and waits for IRQ.
 * The final software product works in interrupt mode.
 *
 * \param [out] slicesReady The value saved in this pointer contains the number of encoded slices in the hardware output buffer.
 * \param [out] status_register The value saved in this pointer contains the bit map, which indicates the status of the encoding work.
 * \return <tt>EWL_HW_WAIT_OK</tt>
 * \return <tt>EWL_HW_WAIT_ERROR</tt>
 * \return <tt>EWL_HW_WAIT_TIMEOUT</tt>
 */
i32 EWLWaitHwRdy(const void *instance, u32 *slicesReady, void *waitOut,
                 u32 *status_register);


/**@} */

/**
 * \addtogroup ewl_mem
 *
 * @{
 */

/** Allocates n bytes and returns a pointer to the allocated memory.
 *
 *  This function is a wrapper of <tt>malloc()</tt>.
 *
 * \param [in] n The number of bytes to be allocated.
 * \return The address of the allocated memory.
 */
void *EWLmallocExe(u32 n);

/** Allocates memory for an array of n elements of s bytes each and returns a pointer to the allocated memory. The memory is set to zero.
 *
 * This function is a wrapper of <tt>calloc()</tt>.
 *
 * \param [in] n The number of elements.
 * \param [in] s The size of each element, in bytes.
 * \return The address of the allocated memory.
 */
void *EWLcallocExe(u32 n, u32 s);

//FIXME@DBG
#define EWLmalloc(n)    EWLmallocExe(n)
#define EWLcalloc(n, s) EWLcallocExe(n, s)
//#define EWLmalloc(n)    (EWLmallocExe(n) + bk_printf("%s:%d\n", __FILE__, __LINE__) / 1000)
//#define EWLcalloc(n, s) (EWLcallocExe(n, s) +  bk_printf("%s:%d\n", __FILE__, __LINE__) / 1000)

/**  Releases the allocated memory.
 *
 * This function is a wrapper of <tt>free()</tt>.
 */
void EWLfree(void *p);

/** Copies n bytes from memory area s to memory area d.
 *
 * This function is a wrapper of <tt>memcpy()</tt>.
 * \param [in] d The destined memory area for the copied bytes.
 * \param [in] s The source memory area where data values are copied.
 * \param [in] n The size of the copied memory area, in bytes.
 */
mem_ret EWLmemcpy(void *d, const void *s, u32 n);

/** Fills memory with a constant byte.
 *
 * This function is a wrapper of <tt>memset()</tt>.
 * \param [in] d The start memory address where the value is filled.
 * \param [in] c The byte value to be filled.
 * \param [in] n The number of bytes to be filled.
 * */
mem_ret EWLmemset(void *d, i32 c, u32 n);

/** Compares two memory areas.
 *
 * This function is a wrapper of <tt>memcmp()</tt>.
 * \param [in] s1 One memory area to be compared.
 * \param [in] s2 Another memory area to be compared.
 * \param [in] n The size of s1 and s2, in bytes.
 * */
int EWLmemcmp(const void *s1, const void *s2, u32 n);

/** Obtains the information, including address and size, of the input line buffer.
 * \param [in] info Information to identify the input line buffer.
*/
i32 EWLGetLineBufSram(const void *instance, EWLLinearMem_t *info);

/** Allocates memory for the loopback line buffer.
 * \param [in] size The bytes to be allocated.
 * \param [out] info Information to identify the loopback line buffer.
*/
i32 EWLMallocLoopbackLineBuf(const void *instance, u32 size,
                             EWLLinearMem_t *info);


/**@} */

/**
 * \addtogroup ewl_api
 *
 * @{
 */

/**
 * Obtains the register offset of a hardware engine in the subsystem.
 * \param inst An EWL instance.
 * \param client_type The hardware engine.
 *
 * \return  A value equal to 0xffff: the hardware engine inquired is not available.
 * \return A value smaller than 0xffff: the inquired hardware engine is detected and the value equals the register offset
 * of the hardware engine, in bytes.
 */
u16 EWLGetClientOffset(const void *inst, u16 client_type);

/**
 * Obtains the submodule register offset of status cmdbuf in the subsystem.
 * \param inst An EWL instance.
 * \param client_type The hardware engine.
 *
 * \return  A value equal to 0xffff: the hardware engine inquired is not available.
 * \return A value smaller than 0xffff: the inquired hardware engine is detected and the value equals the submodule register offset
 * of status cmdbuf, in bytes.
 */
u16 EWLGetClientVcmdStatusBufOffset(const void *inst, u16 comp_type);

/*
 * Obtains the PSNR and SSIM result. (Only available in the C-model now)
 * \param [in] prof_data Profile data.
 * \param [in] qp Slice quantization parameters used to encode the frame.
 * \param [in] poc Picture order count of the encoded frame.
 */
void EWLTraceProfile(const void *inst, void *prof_data, i32 qp, i32 poc);

/** Obtains the read-only registers loaded by VCMD during initialization.
 * \param [in] offset The offset related to the hardware register base address.
 * \return <tt>EWL_OK</tt> when VCMD is disabled.
 * \return The register values obtained during the VCMD initialization.
 */
u32 EWLReadRegInit(const void *inst, u32 offset);

/** Reserves one VCMD buffer for the current instance.
 *
 * This function requests a VCMD buffer from a system such as the kernel driver.
 * When this function returns, the software fills the buffer with commands to set up the encoding parameters for frame encoding.
 * This function waits internally untill there is a free VCMD buffer available.
 *
 * \param [inout] data
 *   - The <tt>data->vcmdbuf.size</tt> is an input parameter indicating the size required by one VCMD buffer. \n
 *   - The <tt>data->vcmdbuf.id</tt> is filled with the ID of the available VCMD buffer. \n
 *   - The <tt>data->vcmdbuf.status_ba</tt> is filled with the address where the register values read from the hardware are stored.
 * \return <tt>EWL_OK</tt>
 * \return <tt>EWL_ERROR</tt>
 */
i32 EWLReserveCmdbuf(const void *inst, EWLResource_t *data);
/** Puts a VCMD buffer into queue.
 *
 * When a queue is empty, the VCMD buffer is run at once. Otherwise, the VCMD buffer is linked to the end of the queue.
 *
 * \param [in] cmdbufid  The ID to identify the VCMD buffer.
 * \param [in] cmdbuf_size The size of the VCMD buffer.
 * \return <tt>EWL_ERROR</tt>
 * \return <tt>EWL_OK</tt>
 */
i32 EWLLinkRunCmdbuf(const void *inst, u16 cmdbufid, u16 cmdbuf_size);

/** Waits until all commands in the VCMD buffer obtained by <tt>EWLReserveCmdbuf()</tt> are executed.
 *
 * After the VCMD buffer is put into the VCMD buffer queue or run directly by calling <tt>EWLLinkRunCmdbuf()</tt>,
 * this function is called to wait for all commands being executed.
 * Such waiting can be asynchronous, awakened by the IRQ triggered by a VCMD interrupt.
 * It can also be synchronous to poll the VCMD status register.
 *
 * \param [in] cmdbufid The ID to identify the VCMD buffer to run.
 * \param [out] status The value stored in this pointer contains the bit map, which indicates the status of the encoding job.
 * \param [out] slices_rdy The value that indicates how many encoded slices are filled to this address.
 *
 * \return <tt>EWL_OK</tt>
 * \return <tt>EWL_ERROR</tt>
 * \return <tt>EWL_HW_WAIT_ERROR</tt>
 */
i32 EWLWaitCmdbuf(const void *inst, u16 cmdbufid, u32 *status);

/** Copies register values from a VCMD status buffer to mirror registers.
 *
 * \param [in] cmdbufid The ID to identify the VCMD buffer.
 * \param [out] regMirror The address of a buffer to save the register values.
 */
void EWLGetRegsByCmdbuf(const void *ewl, u16 cmdbufid, u32 *regMirror);

/** Releases the VCMD buffer used for encoding the current frame.
 *
 * This function should be called after the hardware finishes processing one VCMD Buffer and
 * software has gotten the encoding information from VCMD status buffer. After calling this API,
 * The VCMD buffer reserved for currrent instance will be returned to system. Then other instance
 * can use it. Software can only access the VCMD buffer between <tt>EWLReserveCmdbuf()</tt> and <tt>EWLReleaseCmdbuf()</tt>.
 *
 * \param [in] cmdbufid The ID to identify the VCMD buffer used for encoding.
 */
i32 EWLReleaseCmdbuf(const void *inst, u16 cmdbufid);

/** Sets the information about the commands stored in the VCMD Buffer.
 *
 * This function is used in VCMD mode only. The information helps the driver
 * estimate the workload and schedule the job in the VCMD Buffer. In parmeter
 * interrupt_ctrl, bit[31] is a flag used to indicate mode.
 * - When mode is 0, interrupt is generated adaptively. In such mode,
 *   - bit[30:0] is the estimating executing time for current job.
 * - When mode flag is 1, interrupt is generated according to bit[0].
 *   - When bit[0] is 0, VCMD will not generate interrupt when run JMP command;
 *   - when bit[0] is 1, VCMD will generate interrupt when run JMP command.
 *
 * \param [in] interrupt_ctrl control how to generate interrupt when execute
 *           VCMD JMP command.
 * \param [in] client_type The hardware engine type.
 * \param [in] priority The priority for the current job.
 */
void EWLSetReserveBaseData(const void *inst, u32 interrupt_ctrl,
                           u32 client_type, u16 priority);

/** Specifies whether VCMD is enabled.
 * \return <tt>1</tt>: Enabled.
 * \return <tt>0</tt>: Disabled.
 */
u32 EWLGetVCMDSupport(const void *inst);


/**@}*/

void EWLSetVCMDMode(const void *inst, u32 mode);
u32 EWLGetVCMDMode(const void *inst);


void EWLAttach(const void *ctx, int slice_idx, i32 vcmd_support);
void EWLDetach();

u32 EWLReleaseEwlWorkerInst(const void *inst);
void EWLClearTraceProfile(const void *inst);
i32 EWLGetVcmdCoreNum(const void *ctx, CLIENT_TYPE client_type);
void EWLReadVcmdPriority(const void *ctx, u32 *priority, u32 client_type);

char *EWLGetDevName(const void *inst);
char *EWLGetMemDevName(const void *inst);
i32 EWLGetConfigRegister(const void *inst, u32 core_id, u32 client_type, u32 offset);
void EWLSetUfbcMode(const void *inst, u32 is_enable, u32 hwId);
#ifdef __cplusplus
}
#endif
#endif /*__EWL_H__*/

/* Included from ewl_memsync.h. */
/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2015 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------

--
--  Abstract : header file of Encoder Wrapper Layer Common Part
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#ifndef __EWL_MEMSYNC_H__
#define __EWL_MEMSYNC_H__

#ifdef __cplusplus
extern "C" {
#endif


/* Included from base_type.h. */

/* Included from ewl.h. */

/*If want to define MEM_SYNC_TEST, the premise is open SUPPORT_MEM_SYNC, otherwise, it doesn't make sense*/
#ifdef SUPPORT_MEM_SYNC
//#define MEM_SYNC_TEST
#else
#ifdef MEM_SYNC_TEST
#undef MEM_SYNC_TEST
#endif
#endif

/**
 * \addtogroup ewl_mem
 *
 * @{
 */
/** \brief The address information of the buffer for test verification of memory synchronization.
 * \n <b>NOTE</b>: This structure can be overwritten. */
struct sync_mem {
  /** \brief The aligned virtual address of the buffer from the host view. */
  u32 *dev_va;
  /** \brief The aligned physical address of the buffer from the host view. */
  ptr_t dev_pa_alloc;
  /** \brief The original virtual address of the buffer in device from the host view. */
  u32 *dev_va_alloc;
};

/** Transfers data between the host and device.
 *
 * This function is available only if memory synchronization is supported.
 *
 * \param [in] mem The buffer information used for synchronization.
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] length The number of bytes to be synchronized.
 * \param [in] dir The direction of transferring data.
 * \return <tt>EWL_OK</tt>
* \return <tt>EWL_PAR_ERROR</tt>
 */
i32 EWLSyncMemData(EWLLinearMem_t *mem, u32 offset, u32 length,
                   enum EWLMemSyncDirection dir);

/** Allocates memory for a buffer in the host.
 *
 * This function is available only if memory synchronization is supported.
 *
 * \param [in] alignment The alignment for allocation.
 * \param [in] size The bytes to be allocated.
 * \param [out] buff The information of the memory allocated as a buffer in the host.
 * \return <tt>EWL_OK</tt>
 */
i32 EWLMemSyncAllocHostBuffer(const void *inst, u32 size, u32 alignment,
                              EWLLinearMem_t *buff);

/** Releases the memory of a buffer in the host.
 *
 * This function is available only if memory synchronization is supported.
 *
 * \param [in] buff  The information of the memory to be released.
 * \return <tt>EWL_OK</tt>
 */
i32 EWLMemSyncFreeHostBuffer(const void *inst, EWLLinearMem_t *buff);

/**@}*/

#ifndef SUPPORT_MEM_SYNC
#define EWLSyncMemData(mem, offset, length, dir) (EWL_OK)
#define EWLMemSyncAllocHostBuffer(inst, size, alignment, buff) (EWL_NOT_SUPPORT)
#define EWLMemSyncFreeHostBuffer(inst, buff) (EWL_NOT_SUPPORT)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __EWL_MEMSYNC_H__ */

/* Stream tracing requires encdebug.h */
#if 0

#ifdef H2_HAVE_ENCTRACE_H
#include "enctrace.h"
#endif
#endif

//#define RCP_PRINT_INTFO

//#define RCP_FIXED_POINT_OPT

#ifdef RCP_FIXED_POINT_OPT
typedef i64 RCP_64bit;
typedef i32 RCP_32bit;
#else
typedef double RCP_64bit;
typedef double RCP_32bit;
#endif


/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/**
 * \addtogroup base_type
 *
 * @{
 */
/** Specifies whether a result is correct. */
typedef enum {
  /** \brief The result is incorrect. */
  ENCHW_NOK = -1,
  /** \brief The result is correct. */
  ENCHW_OK = 0
} bool_e;

/** Specifies a Boolean logic. */
typedef enum {
  /** \brief False. */
  ENCHW_NO = 0,
  /** \brief True. */
  ENCHW_YES = 1
} true_e;
/** @} */

/**
 * \addtogroup api_rc
 *
 * @{
 */
/** Specifies rate control modes. */
typedef enum {
  /** \brief The constrained variable bit rate (CVBR) mode. */
  VCE_RC_CVBR,
  /** \brief The constant bit rate (CBR) mode. */
  VCE_RC_CBR,
  /** \brief The variable bit rate (VBR) mode. */
  VCE_RC_VBR,
  /** \brief The average bit rate (ABR) mode. */
  VCE_RC_ABR,
  /** \brief The constant rate factor (CRF) mode. */
  VCE_RC_CRF,
  /** \brief The constant quantization parameter (CQP) mode. */
  VCE_RC_CQP
} rcMode_e;

/** @} */

typedef enum {
  CU_INFO_LOC_X_BITS = 3,
  CU_INFO_LOC_Y_BITS = 3,
  CU_INFO_SIZE_BITS = 2,
  CU_INFO_MODE_BITS = 1,
  CU_INFO_COST_BITS = 25,
  CU_INFO_INTER_PRED_IDC_BITS = 2,
  CU_INFO_MV_REF_IDX_BITS = 2,
  CU_INFO_MV_X_BITS = 14,
  CU_INFO_MV_Y_BITS = 14,
  CU_INFO_INTRA_PART_BITS = 1,
  CU_INFO_INTRA_PART_BITS_H264 = 2,
  CU_INFO_INTRA_PRED_MODE_BITS = 6,
  CU_INFO_INTRA_PRED_MODE_BITS_H264 = 4,
  CU_INFO_INTRA_DUMMY_BITS = 37,
  CU_INFO_MEAN_BITS = 10,
  CU_INFO_VAR_BITS = 18,
  CU_INFO_VAR_BITS_V3 = 26,
  CU_INFO_QP_BITS = 6,
  CU_INFO_OUTPUT_SIZE = 12,     //96 bits
  CU_INFO_TABLE_ITEM_SIZE = 4,  //32 bits
  CU_INFO_OUTPUT_SIZE_V1 = 26,  //208 bits
  CU_INFO_OUTPUT_SIZE_V2 = 16,  //128 bits
  CU_INFO_OUTPUT_SIZE_V3 = 19,  //152 bits
} cuInfoBits_e;

typedef enum {
  /** YYYY... UUUU... VVVV... */
  ENC_PIXFMT_YUV420_PLANAR = 0,
  /** YYYY... UVUVUV... */
  ENC_PIXFMT_YUV420_SEMIPLANAR = 1,
  /** YYYY... VUVUVU... */
  ENC_PIXFMT_YUV420_SEMIPLANAR_VU = 2,
  /** YUYVYUYV... */
  ENC_PIXFMT_YUV422_INTERLEAVED_YUYV = 3,
  /** UYVYUYVY... */
  ENC_PIXFMT_YUV422_INTERLEAVED_UYVY = 4,
  /** 16-bit RGB 16bpp */
  ENC_PIXFMT_RGB565 = 5,
  /** 16-bit RGB 16bpp */
  ENC_PIXFMT_BGR565 = 6,
  /** 15-bit RGB 16bpp */
  ENC_PIXFMT_RGB555 = 7,
  /** 15-bit RGB 16bpp */
  ENC_PIXFMT_BGR555 = 8,
  /** 12-bit RGB 16bpp */
  ENC_PIXFMT_RGB444 = 9,
  /** 12-bit RGB 16bpp */
  ENC_PIXFMT_BGR444 = 10,
  /** 24-bit RGB 32bpp */
  ENC_PIXFMT_RGB888 = 11,
  /** 24-bit RGB 32bpp */
  ENC_PIXFMT_BGR888 = 12,
  /** 30-bit RGB 32bpp */
  ENC_PIXFMT_RGB101010 = 13,
  /** 30-bit RGB 32bpp */
  ENC_PIXFMT_BGR101010 = 14,
  /** YYYY... UUUU... VVVV... */
  ENC_PIXFMT_YUV420_PLANAR_10BIT_I010 = 15,
  /** YYYY... UVUVUV... */
  ENC_PIXFMT_YUV420_PLANAR_10BIT_P010 = 16,
  /** YYYY... UUUU... VVVV... */
  ENC_PIXFMT_YUV420_PLANAR_10BIT_PACKED_PLANAR = 17,
  /** Y0U0Y1a0a1Y2V0Y3a2a3Y4U1Y5a4a5Y6V1Y7a6a7... */
  ENC_PIXFMT_YUV420_10BIT_PACKED_Y0L2 = 18,
  ENC_PIXFMT_YUV420_PLANAR_8BIT_TILE_32_32 = 19,
  ENC_PIXFMT_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4 = 20,
  /** YYYY... UVUVUV... */
  ENC_PIXFMT_YUV420_SEMIPLANAR_8BIT_TILE_4_4 = 21,
  /** YYYY... VUVUVU... */
  ENC_PIXFMT_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4 = 22,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_PLANAR_10BIT_P010_TILE_4_4 = 23,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_SEMIPLANAR_101010 = 24,
  ENC_PIXFMT_YUV422_888 = 25,
  /** YYYY... VUVU... */
  ENC_PIXFMT_YUV420_8BIT_TILE_64_4 = 26,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_UV_8BIT_TILE_64_4 = 27,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_10BIT_TILE_32_4 = 28,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_10BIT_TILE_48_4 = 29,
  /** YYYY... VUVU... */
  ENC_PIXFMT_YUV420_VU_10BIT_TILE_48_4 = 30,
  /** YYYY... VUVU... */
  ENC_PIXFMT_YUV420_8BIT_TILE_128_2 = 31,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_UV_8BIT_TILE_128_2 = 32,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_10BIT_TILE_96_2 = 33,
  /** YYYY... VUVU... */
  ENC_PIXFMT_YUV420_VU_10BIT_TILE_96_2 = 34,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_8BIT_TILE_8_8 = 35,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_10BIT_TILE_8_8 = 36,
  /** YYYY... VVVV... UUUU... */
  ENC_PIXFMT_YVU420_PLANAR = 37,
  /** YYYY... UVUVUV 64x2 tile */
  ENC_PIXFMT_YUV420_UV_8BIT_TILE_64_2 = 38,
  /** YYYY... UVUVUV 64x2 tile */
  ENC_PIXFMT_YUV420_UV_10BIT_TILE_128_2 = 39,
  /** 24-bit RGB 24bpp (MSB)BGR(LSB) */
  ENC_PIXFMT_RGB888_24BIT = 40,
  /** 24-bit RGB 24bpp (MSB)RGB(LSB) */
  ENC_PIXFMT_BGR888_24BIT = 41,
  /** 24-bit RGB 24bpp (MSB)GBR(LSB) */
  ENC_PIXFMT_RBG888_24BIT = 42,
  /** 24-bit RGB 24bpp (MSB)RBG(LSB) */
  ENC_PIXFMT_GBR888_24BIT = 43,
  /** 24-bit RGB 24bpp (MSB)GRB(LSB) */
  ENC_PIXFMT_BRG888_24BIT = 44,
  /** 24-bit RGB 24bpp (MSB)BRG(LSB) */
  ENC_PIXFMT_GRB888_24BIT = 45,
  ENC_PIXFMT_YVU422SP_888 = 46,
  /** YYYY... UUUU... VVVV... */
  ENC_PIXFMT_YUV444_PLANAR = 47,
  /** X8Y8U8V8... */
  ENC_PIXFMT_YUV444_XYUV8888 = 48,
  /** X2Y10U10V10... */
  ENC_PIXFMT_YUV444_XYUV2101010 = 49,
  /** 24-bit RGB 32bpp */
  ENC_PIXFMT_RGBX8888 = 50,
  /** 24-bit RGB 32bpp */
  ENC_PIXFMT_BGRX8888 = 51,
  /** 30-bit RGB 32bpp */
  ENC_PIXFMT_RGBX1010102 = 52,
  /** 30-bit RGB 32bpp */
  ENC_PIXFMT_BGRX1010102 = 53,
  /** YYYY... VUVUVU... */
  ENC_PIXFMT_YVU420_PLANAR_10BIT_P010 = 54,
  ENC_PIXFMT_Y8b = 55,
  ENC_PIXFMT_Y10bWL = 56,
  ENC_PIXFMT_Y10bWH = 57,
  ENC_PIXFMT_YUV420SP10b = 58,
  ENC_PIXFMT_YUV422P10bWL = 59,
  ENC_PIXFMT_FORMAT_MAX
} EncPixelFormat;


/**
 * \addtogroup api_video
 *
 * @{
 */
/** \brief Defines the information of the encoder API version. */
typedef struct {
  /** \brief A major version with achitecture update or product promotion. */
  u32 major;
  /** \brief A minor version with milestone features added. */
  u32 minor;
  /** \brief A micro version with feature improvement or interface changes. */
  u32 micro;
} VceApiVersion;

/** \brief Defines the build information. */
typedef struct {
  /** \brief The software build ID, which equals the changelist number in Perforce. */
  u32 swBuild;
  /** \brief The hardware build ID, which comes from swreg0. */
  u32 hwBuild;
} VceBuildInfo;

/** @} */

/* H.264 collocated mb */
struct h264_mb_col {
  u8 colZeroFlagStore;  // per 2 mbs, bit[0..3] for 1st, bit[4..7] for second
};

/* VLC TABLE */
typedef struct {
  i32 value;  /* Value of bits  */
  i32 number; /* Number of bits */
} table_s;

/* used in stream buffer handling */
typedef struct {
#ifdef TEST_DATA
  struct stream_trace *stream_trace;
#endif
  u8 *stream;           /* Pointer to next byte of stream */
  u32 size;             /* Byte size of stream buffer */
  u32 byteCnt;          /* Byte counter */
  u32 bitCnt;           /* Bit counter */
  u32 byteBuffer;       /* Byte buffer */
  u32 bufferedBits;     /* Amount of bits in byte buffer, [0-7] */
  u32 zeroBytes;        /* Amount of consecutive zero bytes */
  i32 overflow;         /* This will signal a buffer overflow */
  u32 emulCnt;          /* Counter for emulation_3_byte, needed in SEI */
  i32 *table;           /* Video packet or Gob sizes */
  i32 tableSize;        /* Size of above table */
  i32 tableCnt;         /* Table counter of above table */
  u32 bufferedLeftBits; /* Amount of left bits in byte buffer, [0-7] */
} stream_s;

#if 0
/* General tools */
#define ABS(x) ((x) < (0) ? -(x) : (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIGN(a) ((a) < (0) ? (-1) : (1))
#define OUT_OF_RANGE(x, a, b) \
  ((((i32)x) < (a) ? (1) : (0)) || ((x) > (b) ? (1) : (0)))
#define CLIP3(v, min, max) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))
#endif

/* Encoder MB output information defined as a struct.

   NOTE!    By defining this struct we rely that ASIC output endianess
            is configured properly to produce output that is identical
            to the way the compiler constructs this struct. */


typedef struct EncOutForCutree {
  int poc;
  int qp;
  u32 codingType;
  i32 hierarchial_bit_allocation_GOP_size;
  i32 encoded_frame_number;
  u32 dsRatio;
  u32 extDSRatio;
  i32 p0;
  i32 p1;
  u32 cuDataIdx;
  u32 roiMapEnable;
  i8 *pRoiMapDelta;
  ptr_t roiMapDeltaQpAddr;
  u32 roiMapDeltaSize;
  u32 motionScore[2][2];
} encOutForCutree_s;

/**
 * \addtogroup api_video
 *
 * @{
 */

/** \brief Contains the information of a frame that the video encoder outputs for HEVC (H.265). */
struct hevcFramInfo {
  /** \brief The number of bytes encoded. */
  u32 picBytes;
  /** \brief The number of inter 64x64 CUs. */
  u32 inter64x64CuNum;
  /** \brief The number of inter 32x32 CUs. */
  u32 inter32x32CuNum;
  /** \brief The number of inter 16x16 CUs. */
  u32 inter16x16CuNum;
  /** \brief The number of inter 8x8 CUs. */
  u32 inter8x8CuNum;
  /** \brief The number of intra 32x32 CUs. */
  u32 intra32x32CuNum;
  /** \brief The number of intra 16x16 CUs. */
  u32 intra16x16CuNum;
  /** \brief The number of intra 8x8 CUs. */
  u32 intra8x8CuNum;
  /** \brief The number of intra 4x4 CUs. */
  u32 intra4x4CuNum;
  /** \brief The number of TU residual bits. */
  u32 residualBits;
  /** \brief The number of CU mode bits and MV bits. */
  u32 headBits;
  /** \brief The average MAD for all blocks of the input picture, which can be used to measure the
   * space complexity of the input picture. */
  u32 madiVal;
  /** \brief The average MAD for all blocks of predicted residual, which can be used to measure the
   * time complexity of the input picture. */
  u32 madpVal;
  /** \brief The number of LCUs (that is, CTBs) in the frame. */
  u32 LCU_cnt;
  /** \brief The frame SSE.
   *  \n <tt>sseSum[0]</tt>: the frame SSE for the Y component.
   *  \n <tt>sseSum[1]</tt>: the frame SSE for the U component.
   *  \n <tt>sseSum[2]</tt>: the frame SSE for the V component. */
  u64 sseSum[3];
  /** \brief The SSE for a maximum of eight regions.
   *  \n <tt>sse_info[<em>n</em>][0]</tt>: the SSE for the Y component in region <em>n</em>.
   *  \n <tt>sse_info[<em>n</em>][1]</tt>: the SSE for the U component in region <em>n</em>.
   *  \n <tt>sse_info[<em>n</em>][2]</tt>: the SSE for the V component in region <em>n</em>. */
  u64 sse_info[8][3];
  /** \brief The fixed-point PSNR in 16.16 format.
   *  \n <tt>psnrVal[0]</tt>: the fixed-point PSNR for the Y component.
   *  \n <tt>psnrVal[1]</tt>: the fixed-point PSNR for the U component.
   *  \n <tt>psnrVal[2]</tt>: the fixed-point PSNR for the V component. */
  u32 psnrVal[3];
  /** \brief The QP histogram. */
  u32 qp_hist[52];
  /** \brief The POC of the frame. */
  u32 picPoc;
  /** \brief The number of slices in the frame. */
  u32 picSliceNum;
  /** \brief The number of intra blocks in each 8x8 unit. */
  u32 picNumIntra;
  /** \brief The number of inter merge blocks in each 8x8 unit. */
  u32 picNumMerge;
  /** \brief The number of skip blocks in each 8x8 unit. */
  u32 pSkipNum;
  /** \brief The number of ipcm blocks in each 8x8 unit. */
  u32 ipcmNum;
  /** \brief The initial QP of the frame, which refers to the <tt>sliceQp</tt> in the slice header. */
  u8  startQp;
  /** \brief Whether the CUs in the frame are skip CUs.
   *  \n <tt>0</tt>: the CUs in the frame are not skip CUs.
   *  \n <tt>1</tt>: the CUs in the frame are skip CUs. */
  u8  isPSkip;
  /** \brief The type of the frame.
   *  \n For the available values, see Section <em> \ref VCEncPictureCodingType</em>. */
  u8  picType;
  /** \brief The mean QP of the frame. */
  u8  meanQp;
  /** \brief The index of the picture in the current GOP.
   *  \n The pictures are indexed starting from 0 at each IDR frame. */
  u32 gopPicIdx;
  /** \brief The number of the picture. */
  u32 picNum;
  /** \brief The Sum luma of the picture. */
  u64 pixLumaSum;
  /** \brief The Max luma of the picture. */
  u16  pixLumaMax;
  /** \brief The min luma of the picture. */
  u16  pixLumaMin;
};


/** \brief Contains the information of a frame that the video encoder outputs for H.264 (AVC). */
struct h264FramInfo {
  /** \brief The number of bytes encoded. */
  u32 picBytes;
  /** \brief The number of skip MBs. */
  u32 pSkipMbNum;
  /** \brief The number of IPCM MBs. */
  u32 IpcmMbNum;
  /** \brief The number of inter 16x16 MBs. */
  u32 inter16x16MbNum;
  /** \brief The number of inter 16x8 MBs. */
  u32 inter16x8MbNum;
  /** \brief The number of inter 8x16 MBs. */
  u32 inter8x16MbNum;
  /** \brief The number of inter 8x8 MBs. */
  u32 inter8x8MbNum;
  /** \brief The number of intra 16x16 MBs. */
  u32 intra16MbNum;
  /** \brief The number of intra 8x8 MBs. */
  u32 intra8MbNum;
  /** \brief The number of intra 4x4 MBs. */
  u32 intra4MbNum;
  /** \brief The number of residual bits. */
  u32 residualBits;
  /** \brief The number of CU mode bits. */
  u32 headBits;
  /** \brief The average MAD for all blocks of the input picture, which can be used to measure the
   * space complexity of the input picture. */
  u32 madiVal;
  /** \brief The average MAD for all blocks of predicted residual, which can be used to measure the
   * time complexity of the input picture. */
  u32 madpVal;
  /** \brief The number of LCUs (that is, CTBs) in the frame. */
  u32 LCU_cnt;
  /** \brief The fixed-point PSNR in 16.16 precision.
   *  \n <tt>psnrVal[0]</tt>: the fixed-point PSNR for the Y component.
   *  \n <tt>psnrVal[1]</tt>: the fixed-point PSNR for the U component.
   *  \n <tt>psnrVal[2]</tt>: the fixed-point PSNR for the V component. */
  u32 psnrVal[3];
  /** \brief The frame SSE.
   *  \n <tt>sseSum[0]</tt>: the frame SSE for the Y component.
   *  \n <tt>sseSum[1]</tt>: the frame SSE for the U component.
   *  \n <tt>sseSum[2]</tt>: the frame SSE for the V component. */
  u64 sseSum[3];
  /** \brief The SSE for a maximum of eight regions.
   *  \n <tt>sse_info[<em>n</em>][0]</tt>: the SSE for the Y component in region <em>n</em>.
   *  \n <tt>sse_info[<em>n</em>][1]</tt>: the SSE for the U component in region <em>n</em>.
   *  \n <tt>sse_info[<em>n</em>][2]</tt>: the SSE for the V component in region <em>n</em>. */
  u64 sse_info[8][3];
  /** \brief The QP histogram. */
  u32 qp_hist[52];
  /** \brief The POC of the frame. */
  u32 picPoc;
  /** \brief The number of slices in the frame. */
  u32 picSliceNum;
  /** \brief The number of intra blocks in each 8x8 unit. */
  u32 picNumIntra;
  /** \brief The initial QP of the frame, which refers to the <tt>sliceQp</tt> in the slice header. */
  u8  startQp;
  /** \brief Whether the CUs in the frame are skip CUs.
   *  \n <tt>0</tt>: the CUs in the frame are not skip CUs.
   *  \n <tt>1</tt>: the CUs in the frame are skip CUs. */
  u8  isPSkip;
  /** \brief The type of the frame.
   *  \n For the available values, see Section <em> \ref VCEncPictureCodingType</em>. */
  u8  picType;
  /** \brief The mean QP of the frame. */
  u8  meanQp;
  /** \brief The index of the picture in the current GOP.
   *  \n The pictures are indexed starting from 0 at each IDR frame. */
  u32 gopPicIdx;
  /** \brief The number of the picture. */
  u32 picNum;
  /** \brief The Sum luma of the picture. */
  u64 pixLumaSum;
  /** \brief The Max luma of the picture. */
  u16  pixLumaMax;
  /** \brief The min luma of the picture. */
  u16  pixLumaMin;
};
/** @} */

/**
 * \addtogroup api_video
 *
 * @{
 */

/** \brief The prototype of the callback function that is called when a line buffer interrupt is
 * detected during encoding of a frame. */
typedef void (*EncInputLineBufCallBackFunc)(void *pAppData);

/** \brief The prototype of the callback function that is called when a stream segment interrupt is
 * detected during encoding of a frame. */
typedef void (*EncStreamMultiSegCallBackFunc)(void *pAppData);

/** @} */


typedef struct {
  u32 inputLineBufEn;         /* enable input image control signals */
  u32 inputLineBufLoopBackEn; /* input buffer loopback mode enable */
  u32 inputLineBufDepth;      /* input loopback buffer size in mb lines */
  u32 inputLineBufHwModeEn;   /* hw handshake mode */
  u32 amountPerLoopBack;      /* Handshake sync amount for every loopback */
  u32 wrCnt;
  EncInputLineBufCallBackFunc
      cbFunc;     /* call back function for line buffer interrupt */
  void *cbData;   /* call back function data for line buffer interrupt */
  u32 initSegNum; /* The number of segments which input data is stored for SBI */
  u32 sbi_id_0;   /* flexa sbi id 0 */
  u32 sbi_id_1;   /* flexa sbi id 1 */
  u32 sbi_id_2;   /* flexa sbi id 2 */
  u32 segmentUnitHeight;   /* flexa sbi segment unit height */
  u32 enable_slice_irq;
} inputLineBuf_s;

typedef struct {
  u32 streamMultiSegmentMode; /* 0:single segment 1:multi-segment with no sync 2:multi-segment with sw handshake mode*/
  u32 streamMultiSegmentSize;
  u32 streamMultiSegmentAmount; /* total segment amount must be more than 1*/
  u32 streamMultiSegmentOffset;
  u32 rdCnt;
  EncStreamMultiSegCallBackFunc
      cbFunc;   /* call back function for segment ready interrupt */
  void *cbData; /* call back function data for segment ready interrupt */
} streamMultiSeg_s;

/* Reference frame Mv info for tmv */
struct MvInfo {
  u8 pred_mode;
  u8 pred_dir;
  u8 partMode;
  u8 cuSize;
  u8 puIndex;
  short m_iHor[2];
  short m_iVer[2];
  i32 refIdx[2];
  u8 longTermRef[2];
};
#undef Pel
#if 0
typedef       unsigned char       Pel;        ///< 8-bit pixel type
#else
typedef unsigned short Pel;  // 16-bit pixel type
#endif

/* deblock info when multi-tile */
typedef struct {
  //data
  Pel leftY[16];
  Pel leftC[16];

  //ctrl info
  i32 bmvy;
  i32 bmvx;
  i32 fmvy;
  i32 fmvx;
  u8 bmvid;
  u8 fmvid;
  u8 tile_border;  //above or below
  /* HW-SW_size: 0-8, 1-16, 2-32, 3-64 */
  u8 cu_size;
  u8 pu_split_type;
  u8 tu_border;
  u8 pcm_flag;
  u8 block_coded_block_flag;
  u8 block_bi_dir;
  u8 block_qp;
  u8 block_intra;
  /* HW-SW_size: 0-4, 1-8, 2-16, 3-32 */
  u8 tu_size;
} debTileCtx;

typedef struct {  // per CTB
  // control
  u8 sao_filter_type[2];       // luma/chroma; disable/EO/BO
  u8 sao_filter_info[3];       // yuv; eo_class or band position
  u8 left_sao_disable;         // disable sao for PCM
  i8 sao_filter_offset[3][4];  // yuv; offsets

  // data
  Pel leftY[16][4][5];    // [blkIdxY]
  Pel leftC[2][8][4][5];  // [cb/cr][blkIdxY]
} saoTileCtx;

#define DEB_TILE_SYNC_SIZE ((sizeof(debTileCtx)) * 16)
#define SAODEC_TILE_SYNC_SIZE (sizeof(saoTileCtx))
#define SAOPP_TILE_SYNC_SIZE (2 * sizeof(Pel) * 16)

/* the max ctb size allowed for hevc */
#define MAX_CTB_SIZE 64

/* Masks for encOutputMbInfo.mode */
#define MBOUT_9_MODE_MASK 0x0F
#define MBOUT_9_CHROMA_MODE_MASK 0x30
#define MBOUT_9_REFIDX_MASK 0xC0

/* MB output information. Amount of data/mb in 16-bit words / bytes.
   This is how things used to be before Foxtail release.
   Older models rely on these defines.
   For Foxtail data has increased to 48 bytes/MB, this is defined in model. */
#define MBOUT_16 16
#define MBOUT_8 32

/* 16-bit pointer offset to field. */
#define MBOUT_16_BITCOUNT 1
#define MBOUT_16_MV_B0_X 4
#define MBOUT_16_MV_B1_X 5
#define MBOUT_16_MV_B2_X 6
#define MBOUT_16_MV_B3_X 7
#define MBOUT_16_INTRA_SAD 12
#define MBOUT_16_INTER_SAD 13

/* 8-bit pointer offset to field. */
#define MBOUT_8_MODE 0
#define MBOUT_8_NONZERO_CNT 1
#define MBOUT_8_MV_B0_Y 4
#define MBOUT_8_MV_B1_Y 5
#define MBOUT_8_MV_B2_Y 6
#define MBOUT_8_MV_B3_Y 7
#define MBOUT_8_I4X4 16
#define MBOUT_8_INPUT_MAD 28
#define MBOUT_8_LUMA_MEAN 29
#define MBOUT_8_CB_MEAN 30
#define MBOUT_8_CR_MEAN 31

#define MBOUT_8_BOOS_QP 32
#define MBOUT_8_VAR_REC_LUM 34 /* varRecon[0] */
#define MBOUT_8_VAR_INP_LUM 42 /* varInput[0] */

/* allocate 64 bytes for sliceinfo data */
#define SLICEINFO_SIZE (64)

/* General tools */
#define ABS(x) ((x) < (0) ? -(x) : (x))
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#define SIGN(a) ((a) < (0) ? (-1) : (1))
#define CLIP3(x, y, z) ((z) < (x) ? (x) : ((z) > (y) ? (y) : (z)))
#define WIDTH_64_ALIGN(w) (((w) + 63) & (~63))
#define RAND(a, b) (rand() % ((b) - (a) + 1) + (a))
#define SWAP(a, b, type) \
  {                      \
    type tmp;            \
    tmp = a;             \
    a = b;               \
    b = tmp;             \
  }
#define STRIDE(variable, alignment) \
  (((variable) + (alignment) - 1) & (~((alignment) - 1)))

#define WIENER_DENOISE (1)
#if WIENER_DENOISE
#define USE_TOP_CTRL_DENOISE (1)
#define FIX_POINT_BIT_WIDTH (10)
#define SIG_RECI_BIT_WIDTH (12)
#define MB_NUM_BIT_WIDTH (20)

#define SNR_MAX (2 << FIX_POINT_BIT_WIDTH)
#define SNR_MIN (1 << FIX_POINT_BIT_WIDTH)
#define SIGMA_RANGE (20)
#define SIGMA_MAX (30)
#define FILTER_STRENGTH_H (1 << FIX_POINT_BIT_WIDTH)
#define FILTER_STRENGTH_L (1 << (FIX_POINT_BIT_WIDTH - 1))
#define FILTER_STRENGTH_PARAM \
  ((FILTER_STRENGTH_H - FILTER_STRENGTH_L) / SIGMA_RANGE)
#define SIGMA_SMOOTH_NUM (5)
#endif

#define DENOISE_3DNR
#ifdef DENOISE_3DNR
#define DENOISE_3DNR_UV                  (1)            // 1 to enable denoise for chroma
// #define DENOISE_3DNR_UV_MVUSING_YME1N
#define DENOISE_3DNR_UV_MVUSING_YMEMD

#define DENOISE_3DNR_INTRAUSENLM3D       (1)            // 1 to enable denoise for I-frame using ME1N
#define DENOISE_RECALC_INTRABLK_SATD     (0)            // 1 to enable re-calculate intra satd using denoised yuv, will copy deniose yuv back before cu_tree_decision()
                                                        // 0 to copy denoise yuv back for ctu_coding() only
#define DENOISE_PRECODE_1STFRAME         (0)            // pre-code 1st I-slice, using its input copy as reference frame.
#endif

#define MOTION_LAMBDA_SSE_SHIFT 6
#define MOTION_LAMBDA_SAD_SHIFT 5
#define IMS_LAMBDA_SAD_SHIFT 6
#define MOTION_LAMBDA_PSY_SHIFT 6

#define AQ_NONE 0
#define AQ_VARIANCE 1
#define AQ_AUTO_VARIANCE 2
#define AQ_AUTO_VARIANCE_BIASED 3

#define QPFACTOR_FIX_POINT 14
#define LAMBDA_FIX_POINT 10
#define SUBJECT_THRESH_FIX_POINT 6

#define SSIM_FIX_POINT_FOR_8BIT 16
#define SSIM_FIX_POINT_FOR_10BIT 24

#define DOWN_SCALING_MAX_WIDTH 8192
//#define SEARCH_RANGE_ROW_OFFSET_TEST

#define CTB_RC_BUF_NUM 4

#define GMV_STEP_X 64
#define GMV_STEP_Y 16

/**
 * \addtogroup api_video
 *
 * @{
 */
/** \brief The supported maximum number of stream buffers. */
#define MAX_STRM_BUF_NUM 2

/** \brief The supported maximum number of OSD regions. */
#define MAX_OVERLAY_NUM 2//12//FIXME@RAMOPT

/** \brief The supported maximum number of mosaic regions. */
#define MAX_MOSAIC_NUM 2//12//FIXME@RAMOPT

/** \brief The supported maximum number of MB. */
#define MAX_LINE_MB_NUM 2048

/** \brief The supported maximum number of OSD map colors, which is the same as maximum number of OSD regions. */
#define MAX_OSDMAP_COLOR_NUM MAX_OVERLAY_NUM

/** @} */

#define SUBJECTIVE_TUNING
#ifdef SUBJECTIVE_TUNING
#define SUBJECTIVE_LAMBDA_TUNING
#define LA_RC_VBV
#define H264_LA_INTRA_MODE_NON_4x4
#define H264_LA_INTRA_MODE_NON_8x8
#define LA_REFERENCE_USE_INPUT
#define H264_LA_BLOCK_SIZE 2
//#define CUTREE_FRAME_COST_NBORDER
//#define H264_DECIMATE
#define RDOQ_LAMBDA_ADJ
//#define DISABLE_LOWDELAYB_FOR_P
#define BILINEAR_DOWNSAMPLE

#define LA_HEVC_SIMPLE_RDO
#define SUBJECTIVE_INITIAL_QP_TUNING
#define LA_INTRA_BY_SATD
//#define GLOBAL_MV_ON_SEARCH_RANGE
//#define PSY_RDOQ
#endif

// optimazation scabac performance for HW
// #define RDO_SCABAC_SIMPLE_EN

// #define TILE_CMODEL_CTB_NUM
#define TILE_CMODEL_SIMULATION

/* macros to improve hevc quality of psnr */
#define VCE_VIDEO_QUALITY_IMPROVE
#ifdef VCE_VIDEO_QUALITY_IMPROVE
/* tools tuning */
#define HEVC_QUALITY_LA_TUNE
#define RDO_CHECK_ZERO_TU
#define REFINE_MEXN_BINCOST

/* rate control */
#define VBR_RC 2
#define FIRST_I_TUNE 1
#define RC_MODEL_DECAY 1.0
#define RC_MODEL_MIN_DIV 2
#define RC_IFRAME_EST_WITH_DELTA 1

#else
#define VBR_RC 0
#define FIRST_I_TUNE 0
#define RC_MODEL_DECAY 0.5
#define RC_MODEL_MIN_DIV 4
#define RC_IFRAME_EST_WITH_DELTA 0
#endif

#define SRD_ENABLE 1
/** * below are const parameters, not for control *** */
#define RDOQ_LMD_INTRA_FACTOR 0.65
#define RDOQ_LMD_INTER_FACTOR 0.75
#define RDOQ_LMD_INTRA_FACTOR_SCALED                    \
  (u32)(RDOQ_LMD_INTRA_FACTOR * RDOQ_LMD_INTRA_FACTOR * \
        (1 << QPFACTOR_FIX_POINT))
#define RDOQ_LMD_INTER_FACTOR_SCALED                    \
  (u32)(RDOQ_LMD_INTER_FACTOR * RDOQ_LMD_INTER_FACTOR * \
        (1 << QPFACTOR_FIX_POINT))

#define MAX_SEGMENTS 8
static const int segment_delta_qp[] = {-8, -6, -4, -2, 0, 2, 4, 6};

#define CDEF_STRENGTH_NUM 8

#define VCENC_FREE(p) \
  do {                \
    if (p) {          \
      EWLfree(p);     \
      p = NULL;       \
    }                 \
  } while (0)

#ifdef SUPPORT_AV1
//#include "av1enccommon.h"
#endif

#ifdef SUPPORT_VP9
#include "vp9enccommon.h"
#endif

#define MC_SYNC_WORD_BYTES (64 * 2)

#define CH_WIDTH_SHIFT(codedChromaIdc) (codedChromaIdc == VCENC_CHROMA_IDC_444 ? 0 : 1)
#define CH_HEIGHT_SHIFT(codedChromaIdc) ((codedChromaIdc == VCENC_CHROMA_IDC_444 || codedChromaIdc == VCENC_CHROMA_IDC_422) ? 0 : 1)

#define VISUAL_INTRA_BIAS_MAX_STRENGTH 15
#define VISUAL_CHROMA_DETECTION_MAX_STRENGTH 10

#ifdef __cplusplus
}
#endif

#endif

/**
 * Get HW configuration.
 *
 * \param [in] codecFormat The codec format.
 *
 * \return EWLHwConfig_t structure containing the ASIC hardware configuration. Refer to the Hantro
 * EWL API document for the definition of EWLHwConfig_t.
 */
const EWLHwConfig_t *EncGetAsicConfig(u32 codecFormat, const void *ctx);
u32 EncAsicGetAsicHWid(u32 client_type, const void *ctx);

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/
/**
 * \defgroup jpegenc_api JPEG Encoder API
 * This section describes the APIs for the initialization and configuration of the JPEG encoder.
 * @{
 */

/** define the mimimum size of the buffer which is used to store JPEG headers generated by software */
#define JPEGENC_STREAM_MIN_BUF0_SIZE 1024

/** \brief JPEG encoder instance. */
typedef const void *JpegEncInst;


/** Function return values. */
typedef enum {
  JPEGENC_FRAME_READY = 1,             /**< \brief  Frame ready                 */
  JPEGENC_RESTART_INTERVAL = 2,        /**< \brief  Restart interval ready      */
  JPEGENC_OK = 0,                      /**< \brief  Operation succeeds          */
  JPEGENC_ERROR = -1,                  /**< \brief  Error                       */
  JPEGENC_NULL_ARGUMENT = -2,          /**< \brief  Null argument               */
  JPEGENC_INVALID_ARGUMENT = -3,       /**< \brief  Invalid argument            */
  JPEGENC_MEMORY_ERROR = -4,           /**< \brief  Memory error                */
  JPEGENC_INVALID_STATUS = -5,         /**< \brief  Invalid status              */
  JPEGENC_OUTPUT_BUFFER_OVERFLOW = -6, /**< \brief  Output buffer overflow      */
  JPEGENC_EWL_ERROR = -7,              /**< \brief  EWL error                   */
  JPEGENC_EWL_MEMORY_ERROR = -8,       /**< \brief  EWL memory error            */
  JPEGENC_HW_BUS_ERROR = -9,           /**< \brief  Hardware bus error          */
  JPEGENC_HW_DATA_ERROR = -10,         /**< \brief  Hardware data error         */
  JPEGENC_HW_TIMEOUT = -11,            /**< \brief  Hardware timeout            */
  JPEGENC_HW_RESERVED = -12,           /**< \brief  Reserved for hardware       */
  JPEGENC_SYSTEM_ERROR = -13,          /**< \brief  System error                */
  JPEGENC_INSTANCE_ERROR = -14,        /**< \brief  Invalid instance            */
  JPEGENC_HW_RESET = -15,              /**< \brief  Hardware reset              */
  JPEGENC_HW_SLICEINFO_TIMEOUT = -16,  /**< \brief  sliceinfo poll timeout      */
  JPEGENC_HW_UFBC_ERROR = -17          /**< \brief  ufbc error                  */
} JpegEncRet;

/** Picture frame type for initialization. */
typedef enum {
  JPEGENC_YUV420_PLANAR = ENC_PIXFMT_YUV420_PLANAR,       /**< \brief  YYYY... UUUU... VVVV...  */
  JPEGENC_YUV420_SEMIPLANAR = ENC_PIXFMT_YUV420_SEMIPLANAR,       /**< \brief  YYYY... UVUVUV...        */
  JPEGENC_YUV420_SEMIPLANAR_VU = ENC_PIXFMT_YUV420_SEMIPLANAR_VU,    /**< \brief  YYYY... VUVUVU...        */
  JPEGENC_YUV422_INTERLEAVED_YUYV = ENC_PIXFMT_YUV422_INTERLEAVED_YUYV, /**< \brief  YUYVYUYV...              */
  JPEGENC_YUV422_INTERLEAVED_UYVY = ENC_PIXFMT_YUV422_INTERLEAVED_UYVY, /**< \brief  UYVYUYVY...              */
  JPEGENC_RGB565 = ENC_PIXFMT_RGB565,                  /**< \brief  16-bit RGB 16bpp         */
  JPEGENC_BGR565 = ENC_PIXFMT_BGR565,                  /**< \brief  16-bit RGB 16bpp         */
  JPEGENC_RGB555 = ENC_PIXFMT_RGB555,                  /**< \brief  15-bit RGB 16bpp         */
  JPEGENC_BGR555 = ENC_PIXFMT_BGR555,                  /**< \brief  15-bit RGB 16bpp         */
  JPEGENC_RGB444 = ENC_PIXFMT_RGB444,                  /**< \brief  12-bit RGB 16bpp         */
  JPEGENC_BGR444 = ENC_PIXFMT_BGR444,                  /**< \brief  12-bit RGB 16bpp         */
  JPEGENC_RGB888 = ENC_PIXFMT_RGB888,                  /**< \brief  24-bit RGB 32bpp         */
  JPEGENC_BGR888 = ENC_PIXFMT_BGR888,                  /**< \brief  24-bit RGB 32bpp         */
  JPEGENC_RGB101010 = ENC_PIXFMT_RGB101010,               /**< \brief  30-bit RGB 32bpp         */
  JPEGENC_BGR101010 = ENC_PIXFMT_BGR101010,               /**< \brief  30-bit RGB 32bpp         */
  JPEGENC_YUV420_I010 = ENC_PIXFMT_YUV420_PLANAR_10BIT_I010,        /**< \brief  10-bit YUV 24bpp         */
  JPEGENC_YUV420_MS_P010 = ENC_PIXFMT_YUV420_PLANAR_10BIT_P010,     /**< \brief  10-bit YUVSP 24bpp       */
  JPEGENC_YUV420_PLANAR_10BIT_PACKED_PLANAR =
      ENC_PIXFMT_YUV420_PLANAR_10BIT_PACKED_PLANAR,     /**< \brief  10-bit YUV Planar Packed       */
  JPENC_YUV420_10BIT_PACKED_Y0L2 = ENC_PIXFMT_YUV420_10BIT_PACKED_Y0L2,  /**< \brief  YUY YVY Packed */
  JPEGENC_YUV420_PLANAR_8BIT_TILE_32_32 =
      ENC_PIXFMT_YUV420_PLANAR_8BIT_TILE_32_32, /**< \brief  8-bit 32x32 tile        */
  JPEGENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4 =
      ENC_PIXFMT_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4, /**< \brief  8-bit 16x16 tile packed 4  */
  JPEGENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 =
      ENC_PIXFMT_YUV420_SEMIPLANAR_8BIT_TILE_4_4, /**< \brief  YYYY... UVUVUV...        */
  JPEGENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4 =
      ENC_PIXFMT_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4, /**< \brief  YYYY... VUVUVU...        */
  JPEGENC_YUV420_PLANAR_10BIT_P010_TILE_4_4 =
      ENC_PIXFMT_YUV420_PLANAR_10BIT_P010_TILE_4_4,                                /**< \brief  YYYY... UVUV... */
  JPEGENC_YUV420_SEMIPLANAR_101010 = ENC_PIXFMT_YUV420_SEMIPLANAR_101010, /**< \brief  YYYY... UVUV... */
  JPEGENC_YUV422SP_888 = ENC_PIXFMT_YUV422_888, /**< \brief  YYYY... UVUVUV...        */
  JPEGENC_YUV420_UV_8BIT_TILE_128_2 = ENC_PIXFMT_YUV420_UV_8BIT_TILE_128_2, /**< \brief  YYYY... UVUV... */
  JPEGENC_YUV420_8BIT_TILE_8_8 = ENC_PIXFMT_YUV420_8BIT_TILE_8_8, /**< \brief  8-bit 8x8 tile              */
  JPEGENC_YUV420_10BIT_TILE_8_8 =
      ENC_PIXFMT_YUV420_10BIT_TILE_8_8,                     /**< \brief  10-bit 8x8 tile              */
  JPEGENC_YVU420_PLANAR = ENC_PIXFMT_YVU420_PLANAR, /**< \brief  10-bit 8x8 tile              */
  JPEGENC_YUV420_FBC64 = ENC_PIXFMT_YUV420_UV_8BIT_TILE_64_2,  /**< \brief  8-bit 64x2 tile          */
  JPEGENC_YUV420_UV_10BIT_TILE_128_2 =
      ENC_PIXFMT_YUV420_UV_10BIT_TILE_128_2,                    /**< \brief  10-bit 128x2 tile        */
  JPEGENC_RGB888_24BIT = ENC_PIXFMT_RGB888_24BIT, /**< \brief  24-bit RGB 24bpp (MSB) BGR (LSB) */
  JPEGENC_BGR888_24BIT = ENC_PIXFMT_BGR888_24BIT, /**< \brief  24-bit BGR 24bpp (MSB) RGB (LSB) */
  JPEGENC_RBG888_24BIT = ENC_PIXFMT_RBG888_24BIT, /**< \brief  24-bit RBG 24bpp (MSB) GBR (LSB) */
  JPEGENC_GBR888_24BIT = ENC_PIXFMT_GBR888_24BIT, /**< \brief  24-bit GBR 24bpp (MSB) RBG (LSB) */
  JPEGENC_BRG888_24BIT = ENC_PIXFMT_BRG888_24BIT, /**< \brief  24-bit BRG 24bpp (MSB) GRB (LSB) */
  JPEGENC_GRB888_24BIT = ENC_PIXFMT_GRB888_24BIT, /**< \brief  24-bit GRB 24bpp (MSB) BRG (LSB) */
   /** \brief YUV422 semi-planar, YYYY... VUVUVU... */
  JPEGENC_YVU422SP_888 = ENC_PIXFMT_YVU422SP_888,
  //JPEGENC_YUV422_888,                             /**< \brief   YYYY... UVUVUV...        */Todo: double check
  /** YYYY... UUUU... VVVV...  */
  //VCENC_YUV444_PLANAR = ENC_PIXFMT_YUV444_PLANAR, // 444 formats not merged
  /** X8Y8U8V8...*/
  //VCENC_YUV444_XYUV8888 = ENC_PIXFMT_YUV444_XYUV8888,
  /** X2Y10U10V10...*/
  //VCENC_YUV444_XYUV2101010 = ENC_PIXFMT_YUV444_XYUV2101010,
  /** 24-bit RGB 32bpp         */
  JPEGENC_RGBX8888 = ENC_PIXFMT_RGBX8888,
  /** 24-bit RGB 32bpp         */
  JPEGENC_BGRX8888 = ENC_PIXFMT_BGRX8888,
  /** 30-bit RGB 32bpp         */
  JPEGENC_RGBX1010102 = ENC_PIXFMT_RGBX1010102,
  /** 30-bit RGB 32bpp         */
  JPEGENC_BGRX1010102 = ENC_PIXFMT_BGRX1010102,
  /** YYYY... VUVUVU...  */
  JPEGENC_YVU420_PLANAR_10BIT_P010 = ENC_PIXFMT_YVU420_PLANAR_10BIT_P010,
  /** 8-bit YYYY... */
  JPEGENC_Y8b = ENC_PIXFMT_Y8b,
  /** 10-bit YYYY... */
  JPEGENC_Y10bWL = ENC_PIXFMT_Y10bWL,
  /** 10-bit YYYY... */
  JPEGENC_Y10bWH = ENC_PIXFMT_Y10bWH,
  /** 10-bit YUV420SP10b YYYY...UVUVUV... */
  JPEGENC_YUV420SP10b = ENC_PIXFMT_YUV420SP10b,
  JPEGENC_FORMAT_MAX = ENC_PIXFMT_FORMAT_MAX
} JpegEncFrameType;

/** Specifies picture scan types for pre-processing.
 *
 * This enumeration is valid only if the value of <tt>EWLHwConfig_t.superTileXSupport</tt>
 *  is <tt>0</tt>. */
typedef enum {
  /** raster scan */
  JPEG_RASTER_SCAN = 0,
  /** supertileX scan */
  JPEG_SUPERTILEX_SCAN = 1
} JpegEncPictureScanType;

/** Specifies the mode of rotation for initialization. */
typedef enum {
  JPEGENC_ROTATE_0 = 0,
  JPEGENC_ROTATE_90R = 1, /**< 90-degree rotation clockwise. */
  JPEGENC_ROTATE_90L = 2, /**< 90-degree rotation counter-clockwise. */
  JPEGENC_ROTATE_180 = 3  /**< 180-degree rotation. */
} JpegEncPictureRotation;

/** Specifies the mode of RGB-to-YUV color space conversion. */
typedef enum {
  JPEGENC_RGBTOYUV_BT601 =
      0, /**< Conversion to YUV values without any range change according to Rec. ITU-R BT.601. */
  JPEGENC_RGBTOYUV_BT709 =
      1, /**< Conversion to YUV values without any range change according to Rec. ITU-R BT.709. */
  JPEGENC_RGBTOYUV_USER_DEFINED = 2, /**< User-defined color space conversion. */
  JPEGENC_RGBTOYUV_BT2020 = 3, /**< Conversion according to Rec. ITU-R BT.2020. */
  /** Conversion from full range RGB to limited range YUV according to Rec. ITU-R BT.601. */
   JPEGENC_RGBFULL_TO_YUVLIMIT_BT601 = 4,
  /** (obsolete) Same as JPEGENC_RGBFULL_TO_YUVLIMIT_BT601. */
  JPEGENC_RGBTOYUV_BT601_FULL_RANGE = JPEGENC_RGBFULL_TO_YUVLIMIT_BT601,
  /** (Just for test) Conversion from RGB in range of (0~219) to limited range YUV
   * according to Rec. ITU-R BT.601. */
  JPEGENC_RGBTOYUV_BT601_LIMITED_RANGE = 5,
  /**< Conversion from full range RGB to limited range YUV according to Rec. ITU-R BT.709. */
  JPEGENC_RGBFULL_TO_YUVLIMIT_BT709 = 6,
  /** (obsolete) Same as JPEGENC_RGBFULL_TO_YUVLIMIT_BT709. */
  JPEGENC_RGBTOYUV_BT709_FULL_RANGE = JPEGENC_RGBFULL_TO_YUVLIMIT_BT709,
} JpegEncColorConversionType;

/** Specifies the encoding type. */
typedef enum {
  /** The frame is stored as a whole in the linear memory. */
  JPEGENC_WHOLE_FRAME = 0,
  /** The frame is sliced according to restart intervals, each with an input address. */
  JPEGENC_SLICED_FRAME
} JpegEncCodingType;

/** Specifies the encoding mode. */
typedef enum {
  /** YUV 420 mode. */
  JPEGENC_420_MODE = 0,
  /** YUV 422 mode (pre-processing only supports input formats JPEGENC_YUV422SP_888
  * and JPEGENC_YVU422SP_888). */
  JPEGENC_422_MODE = 1,
  /** Monochrome mode. */
  JPEGENC_MONO_MODE = 2,
  JPEGENC_CNT_MODE
} JpegEncCodingMode;

/** Specifies the unit type of X-coordinate and Y-coordinate. */
typedef enum {
  JPEGENC_NO_UNITS = 0,      /**< No unit, X and Y specify the pixel aspect ratio. */
  JPEGENC_DOTS_PER_INCH = 1, /**< Dots per inch. */
  JPEGENC_DOTS_PER_CM = 2,   /**< Dots per centimetre. */
} JpegEncAppUnitsType;

/** Specifies the type of marker for tables DHT and DQT. */
typedef enum {
   /** Luma and chroma tables are written behind one marker. */
  JPEGENC_SINGLE_MARKER = 0,
  /** Luma and chroma tables are written behind one marker per component. */
  JPEGENC_MULTI_MARKER
} JpegEncTableMarkerType;

/** Specifies the encoding format of thumbnail. */
typedef enum {
  /** Thumbnail encoded using JPEG format. */
  JPEGENC_THUMB_JPEG = 0x10,
  /** Thumbnail stored using one byte per pixel. */
  JPEGENC_THUMB_PALETTE_RGB8 = 0x11,
  /** Thumbnail stored using three bytes per pixel. */
  JPEGENC_THUMB_RGB24 = 0x13
} JpegEncThumbFormat;

/** Specifies the RC mode. */
typedef enum {
  JPEGENC_SINGLEFRAME = 0, /**< (Obsolete) Single frame mode. */
  JPEGENC_CBR = 1,         /**< CBR RC mode. */
  JPEGENC_VBR = 2,         /**< VBR RC mode. */
} JpegEncRcMode;

/** Specify parameters type to update or retrieve */
typedef enum {
  /** parameters for size of encoded picture (TBD) */
  JPEGENC_PARAM_SIZE = 0,
  /** parameters for rate control (TBD) */
  JPEGENC_PARAM_RC,
  /** parameters for quality */
  JPEGENC_PARAM_QUALITY,
  /** parameters for ROI Map (TBD) */
  JPEGENC_PRRAM_ROIMAP,
  /** number for parameter type */
  JPEGENC_PARAM_MAX
} JpegEncParamType;

/**@}*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/
/**
 * \defgroup api_info Helper Functions
 *
 * @{
 */

/** \brief Indicates API version information. */
typedef VceApiVersion JpegEncApiVersion;

/** \brief Indicates build information. */
typedef VceBuildInfo JpegEncBuild;

/**@}*/

/** \brief  Thumbnail information. */
typedef struct {
  JpegEncThumbFormat format; /**< \brief Thumbnail format.*/
  u16 width;                  /**< \brief Thumbnail width in pixels. */
  u16 height;                 /**< \brief Thumbnail height in pixels. */
  const void *data;          /**< \brief Thumbnail data. */
  u32 dataLength;            /**< \brief Data amount in bytes. */
} JpegEncThumb;

/** \brief  Configures RGB-to-YUV color space conversion. For how the following parameters
  * are used, see Section <tt> \ref ss_csc</tt>. */
typedef struct {
  /** \brief Pre-defined color conversion types. Besides the user-defined type,
   * some standard color conversion types are defined for setting coefficients quickly. */
  JpegEncColorConversionType type;
  u16 coeffA;     /**< \brief  User-defined color space conversion coefficient A. */
  u16 coeffB;     /**< \brief  User-defined color space conversion coefficient B. */
  u16 coeffC;     /**< \brief  User-defined color space conversion coefficient C. */
  u16 coeffE;     /**< \brief  User-defined color space conversion coefficient E. */
  u16 coeffF;     /**< \brief  User-defined color space conversion coefficient F. */
  u16 coeffG;     /**< \brief  User-defined color space conversion coefficient G. */
  u16 coeffH;     /**< \brief  User-defined color space conversion coefficient H. */
  u16 LumaOffset; /**< \brief  User-defined color space conversion parameter LumaOffset. */
} JpegEncColorConversion;

/** \brief  Indicates encoder configuration. */
typedef struct {
  /** \brief  The number of pixels per line in an input picture. */
  u32 inputWidth;
  /** \brief  The number of lines in an input picture. */
  u32 inputHeight;
  /** \brief  The horizontal offset of top-left encoded picture relative to
   * the top-left corner of an input picture. */
  u32 xOffset;
  /** \brief  The vertical offset of top-left encoded picture relative to the top-left corner of an input picture. */
  u32 yOffset;
  /** \brief  The width of an encoded picture. */
  u32 codingWidth;
  /** \brief  The height of an encoded picture. */
  u32 codingHeight;
  /** \brief  The restart interval in MCU rows. */
  u32 restartInterval;

  /** \brief the quality factor from 1 to 100
   * when quality factor is -1, use qLevel */
  i32 quality;

  /** \brief  The quantization level.\n
   * Levels 0 through 9 use the predefined quantization table. \n
   * Level 10 uses the external quantization table specified in <tt>qTableLuma</tt> and <tt>qTableChroma</tt>. */
  u32 qLevel;
  /** \brief Quantization table for luminance, which is used when the quantization level is 10. */
  const u8 *qTableLuma;
  /** \brief Quantization table for chrominance, which is used when the quantization level is 10. */
  const u8 *qTableChroma;

  /** \brief Two 8x8 filters defined for luma and chroma non-ROI regions. */
  u8 filter[128];
  u32 enableRoimap; /**< \brief  Status of the ROI map feature. */
  u32 nonRoiLevel;  /**< \brief  Non-ROI filter level (0 to 9). */

  JpegEncFrameType frameType; /**< \brief  Input frame in YUV or RGB format. */
  /** \brief  Whether to enable RGB-to-YUV conversion. \n <tt>0</tt>: Disable. \n <tt>1</tt>: Enable. */
  JpegEncColorConversion colorConversion;
  /** \brief  Picture rotation mode. */
  JpegEncPictureRotation rotation;
  /** \brief  Whether to enable mirror. \n <tt>0</tt>: Disable. \n <tt>1</tt>: Enable. */
  u32 mirror;
  /** \brief  Encoding type, which can be either whole frame or restart interval. */
  JpegEncCodingType codingType;

  /** \brief  Encoding mode, which can be YUV420, monochrome, or YUV422 coding (pre-processing only supports one special YUV422 format). */
  JpegEncCodingMode codingMode;
  /** \brief  Unit for X and Y density in APP0. */
  JpegEncAppUnitsType unitsType;
  /** \brief  Specifies picture scan types for pre-processing. */
  JpegEncPictureScanType scanType;

  /** \brief  Table marker type. */
  JpegEncTableMarkerType markerType;
  u32 xDensity;   /**< \brief  The horizontal pixel density. */
  u32 yDensity;   /**< \brief  The vertical pixel density. */
  u32 comLength;  /**< \brief  The comment header length. */
  const u8 *pCom; /**< \brief  The comment header pointer. */

  /* low latency */
  u32 inputLineBufEn;         /**< \brief  Whether to enable input picture control signals. \n <tt>0</tt>: Disable. \n <tt>1</tt>: Enable.*/
  u32 inputLineBufLoopBackEn; /**< \brief  Whether to enable input buffer loopback.  \n <tt>0</tt>: Disable. \n <tt>1</tt>: Enable. */
  u32 inputLineBufDepth;      /**< \brief  Input buffer depth in MCU rows. */
  u32 inputLineBufHwModeEn;   /**< \brief  Whether to enable hardware handshake.  \n <tt>0</tt>: Disable. \n <tt>1</tt>: Enable. */
  u32 amountPerLoopBack; /**< \brief  Synchronous handshake amount for every loopback. */

  /** \brief  Callback function. */
  EncInputLineBufCallBackFunc inputLineBufCbFunc;
  void *inputLineBufCbData; /**< \brief  Callback function data. */
  /** \brief  The hash type for verifying stream correctness.
   * \n <tt>0</tt>: Disabled. \n <tt>1</tt>: CRC32 \n <tt>2</tt>: Checksum. */
  u32 hashType;

  /** \brief Whether the constant chroma control is enabled. \n If enabled, chroma is encoded as constant values.
   * \n <tt>0</tt>: Disabled. \n <tt>1</tt>: Enabled. */
  i32 constChromaEn;
  /** \brief The constant value for the U component, when constant chroma control is enabled. */
  u32 constCb;
  /** \brief The constant value for the V component, when constant chroma control is enabled. */
  u32 constCr;

  /** \brief Whether to enable lossless mode. \n If enabled, JPEG lossless mode (SOF3) is used.
   * \n <tt>0</tt>: Disable. \n <tt>1</tt>: Enable. */
  u32 losslessEn;
  /** \brief The predictor mode used for JPEG lossless processing.
   * The value of the nominator of frame rate ranges from 0 to 7. */
  u32 predictMode;
  /** \brief A point transform value used for lossless processing. */
  u32 ptransValue;

  /* Rate control*/
  u32 targetBitPerSecond; /**< \brief  Target bit per second. */

  /* Rate control for MJPEG purpose */
  /** \brief  The numerator of the frame rate. Value range [1, 1048575]. */
  u32 frameRateNum;
   /** \brief  The denominator of the frame rate. Value range [1, frameRateNum].
    * \n The frame rate is the result of frameRateNum divided by frameRateDenom in frames per second. */
  u32 frameRateDenom;
  /** \brief The minimum QP allowed during rate control for motion JPEG. */
  u32 qpmin;
  /** \brief The maximum QP allowed during rate control for motion JPEG. */
  u32 qpmax;
  /** \brief  Fixed QP without rate control. */
  i32 fixedQP;
  /** \brief Rate control mode. */
  JpegEncRcMode rcMode;
  /** \brief Obsolete. */
  i32 picQpDeltaMin;
  /** \brief Obsolete. */
  i32 picQpDeltaMax;
  /** \brief Stride used for input picture width. */
  u32 exp_of_input_alignment;

  /** \brief Slice node index. */
  u32 slice_idx;

  /* stream multi-segment */

  /** \brief Stream multi-segment control mode.
   * \n <tt>0</tt>: Segment disabled.
   * \n <tt>1</tt>: (Reserved) Multi-segment with loopback and no synchronous control.
   * \n <tt>2</tt>: (Reserved) Multi-segment with loopback and software synchronous control.
   * \n <tt>3</tt>: Multi-segment without loopback and no synchronous control. */
  u32 streamMultiSegmentMode;
  /** \brief The total number of stream multi-segment buffers. \n
   * The number must be more than one. \n
   * When loopback is enabled, streams should be written from the start address after the last segment is filled. */
  u32 streamMultiSegmentAmount;
  /** \brief Stream segment buffer size. \n
   * Callback is triggered when one segment is filled. */
  u32 streamMultiSegmentSize;
  /** \brief Callback function is triggered when one stream segment is filled. */
  EncStreamMultiSegCallBackFunc streamMultiSegCbFunc;
  /** \brief Callback data context for stream multi-segment callback function. */
  void *streamMultiSegCbData;

  /** \brief  The maximum AXI burst length. */
  u32 burstMaxLength;
  /** \brief  AXI alignment control.
   * \n bit [31:28] <tt>AXI_burst_align_wr_common</tt>.
   * \n bit [27:24] <tt>AXI_burst_align_wr_stream</tt>.
   * \n bit [23:20] <tt>AXI_burst_align_wr_chroma_ref</tt>.
   * \n bit [19:16] <tt>AXI_burst_align_wr_luma_ref</tt>.
   * \n bit [15:12] <tt>AXI_burst_align_rd_common</tt>.
   * \n bit [11:8] <tt>AXI_burst_align_rd_prp</tt>.
   * \n bit [7:4] <tt>AXI_burst_align_rd_ch_ref_prefetch</tt>.
   * \n bit [3:0] <tt>AXI_burst_align_rd_lu_ref_prefetch</tt>. */
  u32 AXIAlignment;

  /** \brief Interrupt request type mask.
   * \n swreg1 bit 8 <tt>irq_type_sw_reset_mask</tt>.
   * \n swreg1 bit 7 <tt>irq_type_fuse_error_mask</tt>.
   * \n swreg1 bit 6 <tt>irq_type_buffer_full_mask</tt>.
   * \n swreg1 bit 5 <tt>irq_type_bus_error_mask</tt>.
   * \n swreg1 bit 4 <tt>irq_type_timeout_mask</tt>.
   * \n swreg1 bit 3 <tt>irq_type_strm_segment_mask</tt>.
   * \n swreg1 bit 2 <tt>irq_type_line_buffer_mask</tt>.
   * \n swreg1 bit 1 <tt>irq_type_slice_rdy_mask</tt>.
   * \n swreg1 bit 0 <tt>irq_type_frame_rdy_mask</tt>. */
  u32 irqTypeMask;

  /* overlay controls */
  /** \brief  The number of OSD input frames recorded to support the roll back feature. */
  u32 olFrameNum[MAX_OVERLAY_NUM];
  /** \brief Whether the control of each OSD area is enabled.
   * \n <tt>0</tt>: Disabled.
   * \n <tt>1</tt>: Enabled. */
  u32 olEnable[MAX_OVERLAY_NUM];
  /** \brief The input formats of each OSD area.
    *\n <tt>0</tt>: ARGB8888.
    *\n <tt>1</tt>: NV12.
    *\n <tt>2</tt>: Bitmap. */
  u32 olFormat[MAX_OVERLAY_NUM];
  /** \brief The global alpha value of each OSD area, not used for ARGB8888. */
  u32 olAlpha[MAX_OVERLAY_NUM];
  /** \brief Input source width of an OSD area. */
  u32 olWidth[MAX_OVERLAY_NUM];
  /** \brief The width of each OSD area after cropping. */
  u32 olCropWidth[MAX_OVERLAY_NUM];
  /** \brief The input source height of each OSD area. */
  u32 olHeight[MAX_OVERLAY_NUM];
  /** \brief The height of an OSD area after cropping. */
  u32 olCropHeight[MAX_OVERLAY_NUM];
  /** \brief The top left column offset of an OSD area. */
  u32 olXoffset[MAX_OVERLAY_NUM];
  /** \brief The horizontal offset X, which is the left edge of the cropping region. */
  u32 olCropXoffset[MAX_OVERLAY_NUM];
  /** \brief Top left row offset of an OSD area. */
  u32 olYoffset[MAX_OVERLAY_NUM];
  /** \brief The vertical offest Y, which is the top edge of the cropping region. */
  u32 olCropYoffset[MAX_OVERLAY_NUM];
  /** \brief The input luma stride which is stored for an OSD area. */
  u32 olYStride[MAX_OVERLAY_NUM];
  /** \brief The input chroma stride which is stored for an OSD area. */
  u32 olUVStride[MAX_OVERLAY_NUM];
  /** \brief The global luma Y value for bitmap format. */
  u32 olBitmapY[MAX_OVERLAY_NUM];
  /** \brief The global chroma Cb value for bitmap format. */
  u32 olBitmapU[MAX_OVERLAY_NUM];
  /** \brief The global chroma Cr value for bitmap format. */
  u32 olBitmapV[MAX_OVERLAY_NUM];
  /** \brief Whether the input is in super tile format. (For ARGB8888 format only)
    *\n <tt>0</tt>: Not in super tile format.
    *\n <tt>1</tt>: X-marjor super tile format.
    *\n <tt>2</tt>: Y-major super tile format. */
  u32 olSuperTile[MAX_OVERLAY_NUM];
  /** \brief (Reserved) The width of an OSD area after upscaling. */
  u32 olScaleWidth[MAX_OVERLAY_NUM];
  /** \brief (Reserved) The height of an OSD area after upscaling. */
  u32 olScaleHeight[MAX_OVERLAY_NUM];
  /** \brief The size of the compression table buffer used by DEC400. */
  u32 osdDec400TableSize[MAX_OVERLAY_NUM][3];
  void *osdDec400TableFile[MAX_OVERLAY_NUM];

  /* Mosaic region control */
  /** \brief Whether the Mosaic region is enabled. \n <tt>0</tt>: Disabled. \n <tt>1</tt>: Enabled. */
  u32 mosEnable[MAX_MOSAIC_NUM];
  /** \brief The width of a mosaic region. */
  u32 mosWidth[MAX_MOSAIC_NUM];
  /** \brief The height of a mosaic region. */
  u32 mosHeight[MAX_MOSAIC_NUM];
  /** \brief The top left horizontal offset of a mosaic region. */
  u32 mosXoffset[MAX_MOSAIC_NUM];
  /** \brief The top left vertical offset of a mosaic region. */
  u32 mosYoffset[MAX_MOSAIC_NUM];
  u32 mosSizeIndex;

  /* OSD_MAP */
  u32 osdMapEnable;
  u32 osdMapStride;
  u32 osdMapAlpha[MAX_OSDMAP_COLOR_NUM];
  u32 osdMapY[MAX_OSDMAP_COLOR_NUM];
  u32 osdMapU[MAX_OSDMAP_COLOR_NUM];
  u32 osdMapV[MAX_OSDMAP_COLOR_NUM];
  EWLLinearMem_t *osdMapMem;

  /* flexa sbi */

  /** \brief The FLEXA SBI ID is 0. */
  u32 sbi_id_0;
  /** \brief The FLEXA SBI ID is 1. */
  u32 sbi_id_1;
  /** \brief The FLEXA SBI ID is 2. */
  u32 sbi_id_2;
  /** \brief The FLEXA SBI segment unit height. */
  u32 segmentUnitHeight;

  /** \brief Whether powering down the internal SRAM automatically is enabled.
   * \n <tt>0</tt>: Enabled.
   * \n <tt>1</tt>: (Default) Disabled. */
  u32 sramPowerdownDisable;
  u32 sramPowerdownMode;
  u32 sramPowerdownTimerDiv32;
  /** \brief The debug option for dumpping register. */
  u32 dumpRegister;
  /** \brief Whether external frame buffer compression (UFBC) for input frame buffer is enabled.
   * \n <tt>0</tt>: Disabled.
   * \n <tt>1</tt>: Enabled. */
  u32 ufbcEnable;
  /** \brief Whether YUV transform in external frame buffer compressor is enabled.
   * \n <tt>0</tt>: Disabled.
   * \n <tt>1</tt>: Enabled. */
  u32 ufbcYuvTrans;

  /** \brief The superblock type of UFBC.
   *  \n <tt>0</tt>: 32x8.
   *  \n <tt>1</tt>: 16x16. */
  u32 ufbcBlockType;

  /** \brief Whether to enable block split mode in UFBC.
   *  \n <tt>0</tt>: Block split mode off.
   *  \n <tt>1</tt>: Block split mode on. */
  u32 ufbcBlockSplit;

  /** \brief Whether security mode for encoding is enabled.
   * \n <tt>0</tt>: Disabled.
   * \n <tt>1</tt>: Enabled.*/
  u32 secure_mode;

	/** \brief Whether polling slice information mode of input low latency is enabled.
   * \n <tt>0</tt>: Disabled.
   * \n <tt>1</tt>: Enabled. */
  u32 sliceinfoEn;
  /** \brief the priority of current instance */
  u32 priority;

  /** \brief the device name of enc driver.
   *  \n default is "vsi_vcx" */
  char *enc_dev;

  /** \brief the device name of memalloc driver
   *  \n default is "memalloc" */
  char *mem_dev;
  /** \brief Whether to use vcmd. */
  u32 useVcmd;
  /** \brief lowlatency handshake auto gating disable
   *  \n <tt>0</tt>: enable.
   *  \n <tt>1</tt>: disable. */
  u32 lowlatGatingDisable;
} JpegEncCfg;

/** \brief  Configures encoding input information. */
typedef struct {
  /** \brief  Whether to create a frame header.
   * \n <tt>0</tt>: No.
   * \n <tt>1</tt>: Yes. */
  u32 frameHeader;

  /** \brief The bus address for input luminance (Y) picture.
   * \n Planar format: luminance component.
   * \n Semi-planar format: luminance component.
   * \n Interleaved format: whole picture. */
  ptr_t busLum;
  /** \brief The bus address for input chrominance (U).
   * \n Planar format: U component.
   * \n Semi-planar format: both chrominance.
   * \n Interleaved format: not used. */
  ptr_t busCb;
  /** \brief The bus address for input chrominance (V).
   * \n Planar format: V component.
   * \n Semi-planar format: not used.
   * \n Interleaved format: not used. */
  ptr_t busCr;

  /** \brief The bitmap buffer for ROI definition. Each bit defines one MCU. */
  ptr_t busRoiMap;
  /** \brief Quantization table filter, when 0 is set
   * in the corresponding bit of the ROI map buffer. */
  u8 *filter;
  /** \brief Virtual address for input luminance   (Y). */
  const u8 *pLum;
  /** \brief Virtual address for input chrominance (U). */
  const u8 *pCb;
  /** \brief Virtual address for input chrominance (V). */
  const u8 *pCr;

  /* DEC400 */

  /** \brief  The bus address of DEC400 compression table for luminance input (Y). */
  ptr_t dec400TableBusLum;
  /** \brief  The bus address of DEC400 compression table for chrominance input (U). */
  ptr_t dec400TableBusCb;
  /** \brief  The bus address of DEC400 compression table for chrominance input (V). */
  ptr_t dec400TableBusCr;
  /** \brief  The virtual address of DEC400 compression table for luminance input (Y). */
  const u8 *dec400TablepLum;
  /** \brief  The virtual address of DEC400 compression table for chrominance input (U). */
  const u8 *dec400TablepCb;
  /** \brief  The virtual address of DEC400 compression table for chrominance input (V). */
  const u8 *dec400TablepCr;
  /** \brief The working mode of DEC400.
   * \n <tt>0</tt>: Disabled (Global bypass). \n <tt>1</tt>: Stream bypass. \n <tt>2</tt>: Enabled. */
  u32 dec400Enable;
  /** \brief Bus adress of sliceinfo data */
  ptr_t sliceinfoBus;
  /* AXIFE */

  /** \brief The working mode of AXI_FE.
   * \n <tt>0</tt>: Disabled.  \n <tt>1</tt>: Normal mode.
   * \n <tt>2</tt>: Bypass. \n <tt>3</tt>: Security mode. */
  u32 axiFEEnable;

  /* output buffers */
  /** \brief  The virtual address of the output stream buffer. */
  u8 *pOutBuf[MAX_STRM_BUF_NUM];
  /** \brief  The bus address of the output stream buffer. */
  ptr_t busOutBuf[MAX_STRM_BUF_NUM];
  /** \brief  The output buffer size in bytes. */
  u32 outBufSize[MAX_STRM_BUF_NUM];

  /** \brief  The number of MCU rows in the input MCU row buffer. */
  u32 lineBufWrCnt;
  /** \brief  The number of segments where input data is stored. */
  u32 initSegNum;

  /* overlay */
  /** \brief Whether to enable one overlay region.
   *  \n <tt>0</tt>: Disable.
   *  \n <tt>1</tt>: Enable. */
  u32 overlayEnable[MAX_OVERLAY_NUM];
  /** \brief The bus address of the luma component buffer of an overlay picture. */
  ptr_t busOlLum[MAX_OVERLAY_NUM];
  /** \brief The bus address of the Cb component buffer of an overlay picture. */
  ptr_t busOlCb[MAX_OVERLAY_NUM];
  /** \brief The bus address of the Cr component buffer of an overlay picture. */
  ptr_t busOlCr[MAX_OVERLAY_NUM];
  /** \brief Whether the overlay picture of region 0 is compressed by DEC400.
   * \n <tt>0</tt>: No.
   * \n <tt>1</tt>: Bypass.
   * \n <tt>2</tt>: Yes. */
  u32 osdDec400Enable[MAX_OVERLAY_NUM];
  /** \brief The bus address of the DEC400 compression table when the overlay picture of region 0 is compressed by DEC400. */
  ptr_t osdDec400TableBase[MAX_OVERLAY_NUM][3];
  /** \brief (For internal use only) The number of pictures counted. */
  u32 picCnt;
  /** \brief OSDMap enable */
  u32 osdMapEnable;
  /** \brief OSD MAP buffer base address*/
  ptr_t osdMapInputAddr;
} JpegEncIn;

/** \brief  Indicates output information. */
typedef struct {
  /** \brief Encoded JFIF size in bytes. */
  u32 jfifSize;
  /** \brief  Unused bytes in buf0 tail*/
  u32 invalidBytesInBuf0Tail;
  /** \brief JPEG header size in bytes */
  u32 headerSize;
} JpegEncOut;

/** \brief Indicates the JPEG rate control parameters. */
typedef struct {
  /** \brief Whether to enable bit rate control by adjusting quantization parameters. Value range [0,1].
   * \n <tt>0</tt>: Disable. \n <tt>1</tt>: Enable. */
  u32 pictureRc;
  /** \brief QP for the next encoded picture when rate control is enabled. Value range [-1, 51].
   * \n The value <tt>-1</tt> indicates rate control calculates the initial QP for the first picture.
   * \n Other values indicate this QP is used as the initial QP for the first picture and rate control calculates the QP for the following pictures. */
  i32 qpHdr;
  u32 qpMin; /**< \brief   Reserved. */
  u32 qpMax; /**< \brief   Reserved. */
  /** \brief  Target bit rate in the unit of bits per second.
   * It is mandatory when RC is enabled. Value range [10000, 60000000]. */
  u32 bitPerSecond;
  /** \brief (Reserved) Whether to enable HRD. \n <tt>0</tt>: Disable. \n <tt>1</tt>: Enable. */
  u32 hrd;
  /** \brief Bit rate control window. Value range [1, 300].
   *  \n The target bit rate is recommended in input frames. */
  u32 bitrateWindow;
  /** \brief Fixed QP value for all intra pictures. Value range [0, 51].
   * \n The value 0 indicates rate control calculates intra QP. */
  u32 fixedIntraQp;
  /** \brief Tolerance of the maximum moving bit rate, which is the maximum limit of the instantaneous bit rate. */
  i32 tolMovingBitRate;
  /** \brief Reserved. */
  i32 monitorFrames;
  /** \brief The minimum picture QP delta. It is used to smooth quality change between pictures. */
  i32 picQpDeltaMin;
  /** \brief The maximum picture QP delta. It is used to smooth quality change between pictures. */
  i32 picQpDeltaMax;
  /** \brief Frame rate. */
  u32 frameRateNum;
  /** \brief The maximum frame rate is the result of <tt>frameRateNum</tt> divided by <tt>frameRateDenom</tt> in frames per second.
   * The actual frame rate is defined by time increment of encoded pictures.*/
  u32 frameRateDenom;
} JpegEncRateCtrl;

/** \brief Parameters to set quality of encoded picture */
typedef struct {
  /** valid quality factor is from 1 to 100. when factor is -1, other method
   * will be used to adjust the quantization table. */
  i32 factor;
} JpegEncQuality;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

/**
 * \addtogroup jpegenc_api JPEG Encoder API
 *
 * @{
 */

/**
 * Initializes the encoder and returns an instance.
 * The configuration contains stream parameters that cannot be changed during encoding.
 *
 * \brief Creates and initializes encoder instance.
 *
 * \param [in] config A pointer to the <tt>JpegEncCfg</tt> structure that contains initial configuration
 *             parameters of the encoder. These parameters cannot be changed during encoding.
 * \param [inout] instAddr A pointer to a space where the new instance pointer of the encoder is stored.
 * \param [in] ctx A pointer to the context which is initialized in application and may be used by EWL. For example,
 *             when the device is open before initialization, the ctx can be used to transfer this data.
 *             If this parameter is not used by the application, its value can be NULL.
 * \returns <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncInit(const JpegEncCfg *pEncCfg, JpegEncInst *instAddr,
                       void *ctx);

/**
 * Releases an encoder instance.
 *
 * \param [in] inst Encoder instance
 */
JpegEncRet JpegEncRelease(JpegEncInst inst);

/**
 * Set parameters used for encoding.
 *
 * \brief Set parameters used for encoding.
 *
 * \param [in] inst Encoder instance
 * \param [in] type of parameters to set
 * \param [in] parameters for corresponding type.
 * \returns  <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncSetParams(JpegEncInst inst, JpegEncParamType type,
                                  void *param);
/**
 * Get parameters used for encoding.
 *
 * \brief Get parameters used for encoding.
 *
 * \param [in] inst Encoder instance
 * \param [in] type of parameters to set
 * \param [out] parameters for corresponding type.
 * \returns  <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncGetParams(JpegEncInst inst, JpegEncParamType type,
                                  void *param);

/**
 * Specifies the encoding configuration.
 * \param [in] inst Encoder instance.
 * \param [in] pEncCfg A pointer to the current encoding configuration.
 * \returns <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncSetPictureSize(JpegEncInst inst, const JpegEncCfg *pEncCfg);

/**
 * Specifies the thumbnail of a JPEG picture.
 *
 * \param [in] inst Encoder instance.
 * \param [in] JpegThumb Thumbnail information.
 * \return <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncSetThumbnail(JpegEncInst inst, const JpegEncThumb *JpegThumb);

/**
 * Generates JPEG JFIF output based on the configuration.
 *
 * \param [in] inst Encoder instance.
 * \param [in] pEncIn A pointer to the input and output buffer, as well as control information.
 * \param [out] pEncOut  A pointer to the encoding result.
 * \return <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncEncode(JpegEncInst inst, const JpegEncIn *pEncIn,
                         JpegEncOut *pEncOut);

/*------------------------------------------------------------------------------
  Encoder API for special application
------------------------------------------------------------------------------*/

/** Generates software markers and enables the hardware to encode one frame or one interval.
 * \param [in] inst Encoder instance.
 * \param [in] pEncIn A pointer to input and output buffer, as well as control information.
 * \param [out] pEncOut A pointer to the encoding result.
 * \return <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncEncodeRun(JpegEncInst inst, const JpegEncIn *pEncIn,
                            JpegEncOut *pEncOut);
/**
 * Waits for the completion of one encoding task.
 * \param [in] inst Encoder instance.
 * \param [out] pEncOut A pointer to the storage space for receiving the encoding result.
 * \return <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncEncodeWait(JpegEncInst inst, JpegEncOut *pEncOut);
/** @} */

/**
 * \defgroup api_ull Low-Latency Control API
 *
 * @{
 */

/**
 * Obtains the number of the encoded input MCU rows of the current picture.
 *
 * \param [in] inst Encoder instance.
 * \return The number of encoded MCU rows.
 */
u32 JpegEncGetEncodedMbLines(JpegEncInst inst);

/**
 * Specifies the number of input buffer MCU rows of the current picture to be processed by the encoder.
 *
 * \param [in] inst Encoder instance.
 * \return <tt> \ref JpegEncRet </tt>
 */
i32 JpegEncSetInputMBLines(JpegEncInst inst, u32 lines);

/** @} */

/**
 * \defgroup api_rc MJPEG Bit Rate Control API
 *
 * @{
 */

/**
 * Obtains rate control configuration.
 *
 * \param [in] inst Encoder instance.
 * \param [in] pRateCtrl A pointer to the storage space for receiving the rate control configuration.
 * \return <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncSetRateCtrl(JpegEncInst inst,
                              const JpegEncRateCtrl *pRateCtrl);
/**
 * Sets rate control configuration.
 *
 * \param [in] inst Encoder instance.
 * \param [out] pRateCtrl A pointer to the storage space for receiving the rate control configuration.
 * \return <tt> \ref JpegEncRet </tt>
 */
JpegEncRet JpegEncGetRateCtrl(JpegEncInst inst, JpegEncRateCtrl *pRateCtrl);

/** @} */

/**
 * \addtogroup api_info Helper API
 *
 * @{
 */

/**
 * Obtains function version information.
 *
 * \return Function version information. */
JpegEncApiVersion JpegEncGetApiVersion(void);

/**
 * Obtains build information about the specified core.
 *
 * \param [in] id Core ID.
 * \param [in] ctx Context used to initialize the instance.
 * \return Build information.
 */
JpegEncBuild JpegEncGetBuild(u32 id, const void *ctx);

/**
 * Specifies whether a core supports a specified feature.
 *
 * \param [in] feature Configuration of the core.
 * \param [in] cfg Required configuration.
 * \return <tt>1</tt>: Features indicated in cfg are supported.
 * \return <tt>0</tt>: Not supported.
 */
int JpegEncCoreHasFeatures(const EWLHwConfig_t *feature, EWLHwConfig_t *cfg);

/**
 * Obtains the bit depth of an input format.
 *
 * \param [in] type Picture format.
 * \return Bit depth (bits per pixel) of the specified format.
 */
u32 JpegEncGetBitsPerPixel(JpegEncFrameType type);

/**
 * Obtains the stride in bytes based on the specified alignment and format.
 *
 * \param [in] width Picture width in pixels.
 * \param [in] input_format Picture format.
 * \param [out] luma_stride A pointer to fill the required luma stride in bytes.
 * \param [out] chroma_stride A pointer to fill the required chroma stride in bytes.
 * \param [in] input_alignment User-specified alignment requirement in bytes.
 *     If there is no requirement on the alignment, this parameter can be set to zero.
 * \return Bytes of one pixel.
 */
u32 JpegEncGetAlignedStride(int width, i32 input_format, u32 *luma_stride,
                            u32 *chroma_stride, u32 input_alignment, u32 scan_type);
u32 JpegEncGetUfbcAlignment(u32 input_format, u32 blockType);
u32 JpegEncPreGetHwFormat(u32 input_format);



/*------------------------------------------------------------------------------
    Encoder API for set/get luma chroma size and QpHdr
------------------------------------------------------------------------------*/

/** Obtains the size information of the luma input buffer in an instance.
 *
 * \param [in] inst Encoder instance.
 * \param [out] lumSize A pointer to the storage space for receiving the size of the input luma buffer.
 * \param [out] dec400LumaTableSize A pointer to the storage space for receiving the DEC400 table size of the input luma buffer.
 */
JpegEncRet JpegGetLumaSize(JpegEncInst inst, u64 *lumaSize,
                           u64 *dec400LumaTableSize);

/** Sets the size of the luma input buffer in an instance.
 *
 * \param [in] inst Encoder instance.
 * \param [in] lumSize The size of the corresponding input luma buffer.
 * \param [out] dec400LumaTableSize The DEC400 table size of the corresponding input luma buffer.
 */
JpegEncRet JpegSetLumaSize(JpegEncInst inst, u64 lumaSize,
                           u64 dec400LumaTableSize);

/** Obtains the size of the chroma input buffer in an instance.
 *
 * \param [in] inst Encoder instance.
 * \param [out] chromaSize A pointer to the storage space for receiving the size of the corresponding input chroma buffer.
 * \param [out] dec400ChrTableSize A pointer to the storage space for receiving the DEC400 table size of the corresponding input chroma buffer.
 */
JpegEncRet JpegGetChromaSize(JpegEncInst inst, u64 *chromaSize,
                             u64 *dec400ChrTableSize);

/** Sets the size of the input buffer in an instance.
 *
 * \param [in] inst Encoder instance.
 * \param [in] chromaSize A pointer to the storage space for receiving the size of the corresponding input chroma buffer.
 * \param [in] dec400ChrTableSize A pointer to the storage space for receiving the DEC400 table size of the corresponding input chroma buffer.
 */
JpegEncRet JpegSetChromaSize(JpegEncInst inst, u64 chromaSize,
                             u64 dec400ChrTableSize);

/** Obtains the QP used for encoding the current frame. The QP identifies picture encoding quality.
 *
 * \param [in] inst Encoder instance.
 * \return <tt>0</tt> to <tt>51</tt>, with <tt>0</tt> indicating the best quality and <tt>51</tt> the poorest quality.
 */
int JpegGetQpHdr(JpegEncInst inst);

/*------------------------------------------------------------------------------
    8. Encoder API for get ewl
------------------------------------------------------------------------------*/

/**
 * Obtains the EWL instance, which is used to allocate linear buffers.
 *
 * \param [in] inst Encoder instance.
 * \return EWL instance internally initialized.
 */
const void *JpegEncGetEwl(JpegEncInst inst);

/**
 * Obtains OSD slice information.
 *
 * \param [in] inst Encoder instance
 * \param [in] pEncIn A pointer to the input and output buffer, as well as control information.
 * \param [in] restartInterval MCU rows to be encoded as one interval except for the last one.
 * \param [in] partialCoding Enabling status of partial encoding.
 * \param [in] slice Interval index of the current partial encoding.
 * \param [in] sliceRows MCU rows for the current interval.
 */
void JpegEncGetOverlaySlice(JpegEncInst inst, JpegEncIn *pEncIn,
                            i32 restartInterval, i32 partialCoding, i32 slice,
                            i32 sliceRows, u32 yOffset);
void JpegEncGetOSDMapSlice(JpegEncInst inst, JpegEncIn *pEncIn,
                            i32 restartInterval, i32 partialCoding, i32 slice, u32 yOffset);

/**
 * Obtains encoding performance.
 *
 * \param [in] inst Encoder instance.
 * \return Cycles used to encode one frame or one interval.
 */
u32 JpegEncGetPerformance(JpegEncInst inst);

/**
 * Specifies whether JPEG SBI is supported.
 *
 * \param [in] inst Encoder instance.
 * \return <tt>1</tt>: Supported.
 * \return <tt>0</tt>: Not supported.
 */
u32 JpegGetSbiSupport(JpegEncInst inst);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
