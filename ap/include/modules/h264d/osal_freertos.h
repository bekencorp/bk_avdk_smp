/*------------------------------------------------------------------------------
--         Copyright (c) 2015, VeriSilicon Inc. All rights reserved           --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/


#ifndef _OSAL_FREERTOS_H_
#define _OSAL_FREERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

// Map standard C assert to BK_ASSERT
#ifndef assert
#define assert(exp) BK_ASSERT(exp)
#endif

// Map standard C printf to BK_DUMP_OUT
#ifndef printf
#define printf(...) BK_DUMP_OUT(__VA_ARGS__)
#endif

#ifndef _HAVE_PTHREAD_H
#define _HAVE_PTHREAD_H
#endif

// OSAL adapter layer: Map pthread types to OS types
// Define these types when using FreeRTOS (non-simulator) BEFORE any other includes
// This ensures pthread_t and other types are available for osal_pid and other structures
// For FREERTOS_SIMULATOR on Linux, pthread types come from <pthread.h> (included later)
// For FREERTOS_SIMULATOR on Windows, pthread types come from FreeRTOS_POSIX (included later)
// For non-simulator FreeRTOS, define types here using OS abstraction layer
#if defined(__FREERTOS__) && !defined(FREERTOS_SIMULATOR)


// File open flags
#define	O_RDONLY	0		/* +1 == FREAD */
#define	O_WRONLY	1		/* +1 == FWRITE */
#define	O_RDWR		2		/* +1 == FREAD|FWRITE */
#define O_CREAT		0x0200	/* create if nonexistant */
#define O_TRUNC		0x0400	/* truncate to zero length */
#define O_APPEND	0x0800	/* set append mode */
#define O_SYNC		0x2000	/* synchronous writes */
#define O_NONBLOCK	0x4000	/* non-blocking I/O */

// Memory protection flags for mmap
#define PROT_READ	0x1		/* page can be read */
#define PROT_WRITE	0x2		/* page can be written */
#define PROT_EXEC	0x4		/* page can be executed */
#define PROT_NONE	0x0		/* page can not be accessed */

// Memory mapping flags for mmap
#define MAP_SHARED	0x01	/* share changes */
#define MAP_PRIVATE	0x02	/* changes are private */
#define MAP_FIXED	0x10	/* interpret addr exactly */

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

#elif defined(__linux__) /*Linux Simulator*/
/*************************************************************************************************** 
Need to include some basic FreeRTOS header files when Freertos simulator in Linux, at the same time,
the related makefile files need to include the FreeRTOS path, like the below:

Exists variale:
  FREERTOS_DIR ?= $(CMBASE)/software/../../../FreeRTOS/
    FreeRTOS_Kernel
    lib
  FreeRTOSDir = $(FREERTOS_DIR)/FreeRTOS_Kernel
  ifeq ($(USE_FREERTOS_SIMULATOR), y)
    INCLUDE += -I$(FreeRTOSDir)/Source/include \
              -I$(FreeRTOSDir)/Source/portable/Linux/GCC/Posix \
              -I$(FreeRTOSDir)/Source/portable/Linux
  endif
And need to define the macro "__FREERTOS__"
***************************************************************************************************/
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
#include <limits.h> /* NAME_MAX */
#endif /* _WIN32 */

#else //other os, example FREERTOS (Need macro __FREERTOS__)

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
#elif defined(__linux__) /*Linux simulator */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#endif /* _WIN32 */

#else //Not FreeRTOS simulator, FreeRTOS and other os
// Use OS abstraction layer - os/os.h is already included at the top
// No need for FreeRTOS_POSIX includes
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
// Do not include standard C library headers (string.h, assert.h, stdlib.h, stdio.h, etc.)
// Use OS abstraction layer functions and common/bk_* functions instead

#ifdef FREERTOS_SIMULATOR
/*file operations */
#if defined(_MSC_VER)
/* MSVS doesn't define off_t, and uses _f{seek,tell}i64. */
/*typedef __int64 off_t;*/
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
 /*Nothing to do */
#endif /* _MSC_VER */

#else //For FreeRTOS, close all the file operations, maybe can use FreeRTOS+FAT
//typedef long long off_t;
#define off64_t off_t
#endif /* FREERTOS_SIMULATOR */

#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
#define putw _putw

typedef unsigned long(__stdcall * THREAD_ROUTINE)(void * Argument);
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
#ifdef CMODEL_DLL_SUPPORT  /* cmodel in dll */
#ifdef _CMODEL_   /* This macro will be enabled when compiling cmodel libs. */
#define CMODEL_DLL_API __declspec(dllexport)
#else
#define CMODEL_DLL_API __declspec(dllimport)
#endif /* _CMODEL_ */
#else
/* win non-dll. */
#define CMODEL_DLL_API
#endif /* CMODEL_DLL_SUPPORT */
#endif /* CMODEL_DLL_API */

// PTHREAD_MUTEX_INITIALIZER is already defined in the adapter layer above
// No need to redefine it here
/*
 * pthread_attr_{set}inheritsched
 */
#define PTHREAD_INHERIT_SCHED   0
#define PTHREAD_EXPLICIT_SCHED  1 /* Default */

enum {
  //SCHED_OTHER = 0,
  SCHED_FIFO = 1,
  SCHED_RR,
  SCHED_MIN = SCHED_OTHER,
  SCHED_MAX = SCHED_RR
};

#ifdef _DWL_DEBUG
#define DWL_DEBUG BK_DUMP_OUT  // Use BK_DUMP_OUT from common/bk_assert.h
#else
#define DWL_DEBUG(fmt, _VA_ARGS__) \
        do {                            \
        } while (0)
#endif /* _DWL_DEBUG */

#elif defined(__linux__)
#define _FILE_OFFSET_BITS 64  // for 64 bit fseeko
#define fseek fseeko
#define int16 int16_t
#define int64 int64_t
#define __int64 long long

#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif

#ifdef _DWL_DEBUG
#define DWL_DEBUG(fmt, args...) \
    BK_DUMP_OUT(__FILE__ ":%d:%s() " fmt, __LINE__, __func__, ##args)  // Use BK_DUMP_OUT from common/bk_assert.h
#else
#define DWL_DEBUG(fmt, args...) \
	do {                        \
	} while (0)
#endif/* _DWL_DEBUG */
#endif  /* _WIN32 */

#else //For FreeRTOS, close all the file operations, maybe can use FreeRTOS+FAT
//#define fseek(a, b, c) 
// #define int16 short
// #define int64 long long
// #ifndef __int64
// #define __int64 long long
// #endif
#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif /* CMODEL_DLL_API */
/*
* pthread_attr_{set}inheritsched
*/
#define PTHREAD_INHERIT_SCHED   0
#define PTHREAD_EXPLICIT_SCHED  1 /* Default */
#define SCHED_OTHER 0
// enum {
//   // SCHED_OTHER,
//   SCHED_FIFO = 1,
//   SCHED_RR,
//   SCHED_MIN = SCHED_OTHER,
//   SCHED_MAX = SCHED_RR
// };

#ifdef _DWL_DEBUG
#define DWL_DEBUG BK_DUMP_OUT  // Use BK_DUMP_OUT from common/bk_assert.h
#else
#define DWL_DEBUG(fmt, args...) \
        do {                        \
        } while (0)
#endif /* _DWL_DEBUG */

#endif /* FREERTOS_SIMULATOR */

#if 0
#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32) || defined(__linux__)
typedef struct timeval osal_timeval;
#endif /* defined(_WIN32) || defined(__linux__) */

#else //FreeRTOS

//typedef int ssize_t;

typedef struct osal_timeval_
{
//  ssize_t tv_sec;         /* seconds */
  int tv_sec;         /* seconds */
  long    tv_usec;        /* and microseconds */
}osal_timeval;
#define timeval osal_timeval_
#endif /* FREERTOS_SIMULATOR */

struct stream_trace
{
  struct node *next;
  char *buffer;
  char comment[256];
  size_t size;
  void *fp;
  unsigned int cnt;
} ;

typedef void *(*process_main_ptr) (void *arg);

typedef struct {
  int pid;
  pthread_t *tid;
} osal_pid;

/*------------------------------------------------------------------------------
 Function name   : osal_gettimeofday
 Description     : open method
 Parameters      : osal_timeval *tp - Used to store  time of the current time 
                     included the seconds and nanosecnods
                   void *tzp - not used
 Return type     : int
------------------------------------------------------------------------------*/
static av_unused int osal_gettimeofday(osal_timeval *tp, void *tzp)
{
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

#else //FreeRTOS and other os need to implement  by oneself
  //TODO...
  return 0;
#endif /* FREERTOS_SIMULATOR */
}

static av_unused void osal_usleep(unsigned long usec) {
	usleep(usec);
}

static av_unused void osal_open_memstream(struct stream_trace *stream_trace)
{
#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32)
  int fd;
  HANDLE fm, h;
  fd = _fileno(stream_trace->fp);
  h = (HANDLE)_get_osfhandle(fd);

  fm = CreateFileMapping(h, NULL, PAGE_READWRITE | SEC_RESERVE, 0, 16 * 1024 * 1024, NULL);
  if (fm == NULL)
  {
    fprintf(stderr, "Could not access memory space! %sn", strerror(GetLastError()));
    exit(GetLastError());
  }

  GetFileSize(h, stream_trace->size);

  stream_trace->buffer = MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (stream_trace->buffer = NULL)
  {
    fprintf(stderr, "Could not fill memory space! %sn", strerror(GetLastError()));
    exit(GetLastError())
  }
#else
  stream_trace->fp = open_memstream(&stream_trace->buffer, &stream_trace->size);
#endif /* _WIN32 */
#else // For other os
#endif /* FREERTOS_SIMULATOR */
}

/* For cmodel */
static av_unused void * osal_aligned_malloc(unsigned int boundary, unsigned int memory_size)
{
#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
  return _aligned_malloc(memory_size, boundary);
#elif defined(__linux__)
  return memalign(boundary, memory_size);
#endif /* _WIN32 */

#else //FreeRTOS and other os need to implement by oneself
  return NULL;
#endif /* FREERTOS_SIMULATOR */
  return NULL;
}

/* For cmodel */
static av_unused void osal_aligned_free(void * aligned_momory)
{
#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
  _aligned_free(aligned_momory);
#elif defined(__linux__)
  free(aligned_momory);
#endif /* _WIN32 */

#else //FreeRTOS and other os need to implement by oneself
  ;
#endif /* FREERTOS_SIMULATOR */
}
#endif

#ifdef _HAVE_PTHREAD_H

#ifdef SEM_REPLACE_MUTEX

#include "dev_common_freertos.h"

static inline signed int sem_imp_mutex_init(sem_t *mutex, const pthread_mutexattr_t *attr)
{
  sem_init(mutex, 0, 1);

  return 0;
}

static inline signed int sem_imp_cond_init(sem_t *cond, const pthread_condattr_t *attr)
{
  sem_init(cond, 0, 0);

  return 0;
}

static inline signed int sem_imp_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  if(sem_trywait(cond) != 0)
  {
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
