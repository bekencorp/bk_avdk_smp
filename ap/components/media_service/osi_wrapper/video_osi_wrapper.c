#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "video_osi_wrapper.h"
#include "avilib_adp.h"
#if CONFIG_FRAME_BUFFER
#include <components/bk_frame_buffer.h>
#endif

#include <setjmp.h>

#if CONFIG_VFS
#include "bk_posix.h"
#elif CONFIG_FATFS
#include "ff.h"
#endif

extern bk_err_t video_osi_funcs_init(void *config);

static void *malloc_wrapper(size_t size)
{
	return os_malloc(size);
}

static void *zalloc_wrapper(size_t num, size_t size)
{
	return os_zalloc(num * size);
}

static void *realloc_wrapper(void *old_mem, size_t size)
{
	return os_realloc(old_mem, size);
}

static void *psram_malloc_wrapper(size_t size)
{
	return psram_malloc(size);
}

static void *psram_zalloc_wrapper(size_t num, size_t size)
{
	return psram_zalloc(num * size);
}

static void *psram_realloc_wrapper(void *old_mem, size_t size)
{
	return bk_psram_realloc(old_mem, size);
}

static void free_wrapper(void *ptr)
{
	os_free(ptr);
}

static void *memcpy_wrapper(void *out, const void *in, uint32_t n)
{
	return os_memcpy(out, in, n);
}

static void memcpy_word_wrapper(void *out, const void *in, uint32_t n)
{
	return os_memcpy_word(out, in, n);
}

static void assert_wrapper(uint8_t expr, char *expr_s, const char *func)
{
	if (!(expr))
	{
		BK_LOGD(NULL, "(%s) has assert failed at %s.\n", expr_s, func);
		while (1);
	}
}

static uint32_t get_time_wrapper(void)
{
	return rtos_get_time();
}

static int f_open_wrapper(void **fp, const void *path, uint8_t mode)
{
#if (CONFIG_VFS)
	uint32_t flags = 0;

	if (mode & 0x01)
	{
		flags |= O_RDONLY;
	}
	if (mode & 0x02)
	{
		flags |= O_WRONLY;
	}
	if ((mode & 0x03) == 0x03)
	{
		flags |= O_RDWR;
	}

	if (mode & 0x08)
	{
		flags |= O_CREAT | O_TRUNC;
	}
	if (mode & 0x04)
	{
		flags |= O_CREAT | O_EXCL;
	}
	if (mode & 0x30)
	{
		flags |= O_APPEND;
	}
	if (mode & 0x10)
	{
		flags |= O_CREAT;
	}

	int f = open(path, flags);
	if (f < 0) {
		*fp = NULL;
		return -1;
	} else {
		*fp = (void *)f;
		return 0;
	}
#elif (CONFIG_FATFS)
	*fp = os_malloc(sizeof(FIL));
	return f_open((FIL *)*fp, (char *)path, mode);
#else
	return -1;
#endif
}

static int f_close_wrapper(void *fp)
{
#if (CONFIG_VFS)
	int ret = close((int)fp);
	return ret < 0 ? -1 : 0;
#elif (CONFIG_FATFS)
	FRESULT ret = FR_OK;
	ret = f_close((FIL *)fp);
	os_free(fp);
	return ret;
#else
	return -1;
#endif
}

static int f_write_wrapper(void *fp, const void *buff, uint32_t btw, uint32_t *bw)
{
#if (CONFIG_VFS)
	*bw = write((int)fp, (void *)buff, btw);
	return *bw < 0 ? -1 : 0;
#elif (CONFIG_FATFS)
	return f_write((FIL *)fp, (void *)buff, (UINT)btw, (UINT *)bw);
#else
	return -1;
#endif
}

static int f_read_wrapper(void *fp, const void *buff, uint32_t btr, uint32_t *br)
{
#if (CONFIG_VFS)
	*br = read((int)fp, (void *)buff, btr);
	return *br < 0 ? -1 : 0;
#elif (CONFIG_FATFS)
	return f_read((FIL *)fp, (void *)buff, (UINT)btr, (UINT *)br);
#else
	return -1;
#endif
}

static int f_lseek_wrapper(void *fp, uint32_t ofs, uint32_t whence)
{
#if (CONFIG_VFS)
	return lseek((int)fp, ofs, whence);
#elif (CONFIG_FATFS)
	if (whence == SEEK_SET) {
		return f_lseek((FIL *)fp, (FSIZE_t)ofs);
	} else if (whence == SEEK_CUR) {
		return f_lseek((FIL *)fp, f_tell((FIL *)fp) + (FSIZE_t)ofs);
	} else if (whence == SEEK_END) {
		return f_lseek((FIL *)fp, f_size((FIL *)fp) + (FSIZE_t)ofs);
	} else {
		return -1;
	}
#else
	return -1;
#endif
}

static int f_tell_wrapper(void *fp)
{
#if (CONFIG_VFS)
	return (uint32_t)lseek((int)fp, 0, SEEK_CUR);
#elif (CONFIG_FATFS)
	FIL *tmp_fp = (FIL *)fp;
	return (uint32_t)f_tell(tmp_fp);
#else
	return -1;
#endif
}

static int f_size_wrapper(void *fp)
{
#if (CONFIG_VFS)
	BK_LOGD(NULL, "Not support yet, please use the stats function!!!\r\n");
	return -1;
#elif (CONFIG_FATFS)
	FIL *tmp_fp = (FIL *)fp;
	return (int)f_size(tmp_fp);
#else
	return -1;
#endif
}

static int f_unlink_wrapper(const char *path)
{
	if (path == NULL) {
		return -1;
	}
#if (CONFIG_VFS)
	return unlink(path) < 0 ? -1 : 0;
#elif (CONFIG_FATFS)
	return f_unlink(path);
#else
	return -1;
#endif
}

static void *memset_wrapper(void *dst, int value, uint32_t n)
{
	return os_memset(dst, value, n);
}

static void *fb_malloc_wrapper(int heap, uint32_t size)
{
#if CONFIG_FRAME_BUFFER
	frame_buffer_heap_type_t type = (heap == 0) ? MEM_SLAB_HEAP_UNCODED : MEM_SLAB_HEAP_CODED;
	return bk_frame_buffer_malloc(type, size);
#else
	/* No frame-buffer component in this build (e.g. non-media projects that still
	 * link media_service); the fb allocator is unused there. */
	(void)heap;
	(void)size;
	return NULL;
#endif
}

static void fb_free_wrapper(void *frame)
{
#if CONFIG_FRAME_BUFFER
	bk_frame_buffer_free(frame);
#else
	(void)frame;
#endif
}

static int mutex_init_wrapper(void **mutex)
{
	return rtos_init_mutex((beken_mutex_t *)mutex);
}

static int mutex_lock_wrapper(void **mutex)
{
	return rtos_lock_mutex((beken_mutex_t *)mutex);
}

static int mutex_unlock_wrapper(void **mutex)
{
	return rtos_unlock_mutex((beken_mutex_t *)mutex);
}

static int mutex_deinit_wrapper(void **mutex)
{
	return rtos_deinit_mutex((beken_mutex_t *)mutex);
}

static int sem_init_wrapper(void **sem, int max_count)
{
	return rtos_init_semaphore((beken_semaphore_t *)sem, max_count);
}

static int sem_get_wrapper(void **sem, uint32_t timeout_ms)
{
	return rtos_get_semaphore((beken_semaphore_t *)sem, timeout_ms);
}

static int sem_set_wrapper(void **sem)
{
	return rtos_set_semaphore((beken_semaphore_t *)sem);
}

static int sem_deinit_wrapper(void **sem)
{
	return rtos_deinit_semaphore((beken_semaphore_t *)sem);
}

static int queue_init_wrapper(void **queue, const char *name, uint32_t item_size, uint32_t item_count)
{
	return rtos_init_queue((beken_queue_t *)queue, name, item_size, item_count);
}

static int queue_push_wrapper(void **queue, void *item, uint32_t timeout_ms)
{
	return rtos_push_to_queue((beken_queue_t *)queue, item, timeout_ms);
}

static int queue_pop_wrapper(void **queue, void *item, uint32_t timeout_ms)
{
	return rtos_pop_from_queue((beken_queue_t *)queue, item, timeout_ms);
}

static int queue_deinit_wrapper(void **queue)
{
	return rtos_deinit_queue((beken_queue_t *)queue);
}

static int thread_create_wrapper(void **thread, uint8_t priority, const char *name,
	void (*func)(void *), uint32_t stack_size, void *arg)
{
	return rtos_create_thread((beken_thread_t *)thread, priority, name,
		(beken_thread_function_t)func, stack_size, (beken_thread_arg_t)arg);
}

static int thread_delete_wrapper(void **thread)
{
	return rtos_delete_thread((beken_thread_t *)thread);
}

static void delay_ms_wrapper(uint32_t ms)
{
	rtos_delay_milliseconds(ms);
}

static uint32_t get_avi_index_start_addr_wrapper(void)
{
	return AVI_INDEX_START_ADDR;
}

static uint32_t get_avi_index_count_wrapper(void)
{
	return AVI_INDEX_COUNT;
}

static bk_video_osi_funcs_t video_osi_funcs =
{
	.malloc = malloc_wrapper,
	.zalloc = zalloc_wrapper,
	.realloc = realloc_wrapper,
	.psram_malloc = psram_malloc_wrapper,
	.psram_zalloc = psram_zalloc_wrapper,
	.psram_realloc = psram_realloc_wrapper,
	.free = free_wrapper,
	.memcpy = memcpy_wrapper,
	.memcpy_word = memcpy_word_wrapper,

	.log_write = bk_printf_ext,
	.osi_assert = assert_wrapper,
	.get_time = get_time_wrapper,

	.f_open = f_open_wrapper,
	.f_close = f_close_wrapper,
	.f_write = f_write_wrapper,
	.f_read = f_read_wrapper,
	.f_lseek = f_lseek_wrapper,
	.f_tell = f_tell_wrapper,
	.f_size = f_size_wrapper,
	.f_unlink = f_unlink_wrapper,

	.get_avi_index_start_addr = get_avi_index_start_addr_wrapper,
	.get_avi_index_count = get_avi_index_count_wrapper,

	.memset = memset_wrapper,
	.fb_malloc = fb_malloc_wrapper,
	.fb_free = fb_free_wrapper,

	.mutex_init = mutex_init_wrapper,
	.mutex_lock = mutex_lock_wrapper,
	.mutex_unlock = mutex_unlock_wrapper,
	.mutex_deinit = mutex_deinit_wrapper,

	.sem_init = sem_init_wrapper,
	.sem_get = sem_get_wrapper,
	.sem_set = sem_set_wrapper,
	.sem_deinit = sem_deinit_wrapper,

	.queue_init = queue_init_wrapper,
	.queue_push = queue_push_wrapper,
	.queue_pop = queue_pop_wrapper,
	.queue_deinit = queue_deinit_wrapper,

	.thread_create = thread_create_wrapper,
	.thread_delete = thread_delete_wrapper,
	.delay_ms = delay_ms_wrapper,
};

bk_err_t bk_video_osi_funcs_init(void)
{
	bk_err_t ret = BK_OK;
	ret = video_osi_funcs_init(&video_osi_funcs);
	return ret;
}

