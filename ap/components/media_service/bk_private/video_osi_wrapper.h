#pragma once

typedef struct
{
	void *(* malloc)(size_t size);
	void *(* zalloc)(size_t num, size_t size);
	void *(* realloc)(void *old_mem, size_t size);
	void *(* psram_malloc)(size_t size);
	void *(* psram_zalloc)(size_t num, size_t size);
	void *(* psram_realloc)(void *old_mem, size_t size);
	void (* free)(void *ptr);
	void *(* memcpy)(void *out, const void *in, uint32_t n);
	void (* memcpy_word)(void *out, const void *in, uint32_t n);

	void (* log_write)(int level, char *tag, const char *fmt, ...);
	void (* osi_assert)(uint8_t expr, char *expr_s, const char *func);
	uint32_t (* get_time)(void);

	int (* f_open)(void **fp, const void *path, uint8_t mode);
	int (* f_close)(void *fp);
	int (* f_write)(void *fp, const void *buff, uint32_t btw, uint32_t *bw);
	int (* f_read)(void *fp, const void *buff, uint32_t btr, uint32_t *br);
	int (* f_lseek)(void *fp, uint32_t ofs, uint32_t whence);
	int (* f_tell)(void *fp);
	int (* f_size)(void *fp);
	int (* f_unlink)(const char *path);

	uint32_t (* get_avi_index_start_addr)(void);
	uint32_t (* get_avi_index_count)(void);

	/* Extensions for stateful/threaded codec cores (e.g. baf). Opaque handles are
	 * plain void ** so this vtable stays free of any OS header dependency; the
	 * wrapper implementation casts them to the concrete beken_*_t handles. */
	void *(* memset)(void *dst, int value, uint32_t n);

	/* Frame-buffer heap allocator. heap: 0 = uncoded, 1 = coded. */
	void *(* fb_malloc)(int heap, uint32_t size);
	void (* fb_free)(void *frame);

	int (* mutex_init)(void **mutex);
	int (* mutex_lock)(void **mutex);
	int (* mutex_unlock)(void **mutex);
	int (* mutex_deinit)(void **mutex);

	int (* sem_init)(void **sem, int max_count);
	int (* sem_get)(void **sem, uint32_t timeout_ms);
	int (* sem_set)(void **sem);
	int (* sem_deinit)(void **sem);

	int (* queue_init)(void **queue, const char *name, uint32_t item_size, uint32_t item_count);
	int (* queue_push)(void **queue, void *item, uint32_t timeout_ms);
	int (* queue_pop)(void **queue, void *item, uint32_t timeout_ms);
	int (* queue_deinit)(void **queue);

	int (* thread_create)(void **thread, uint8_t priority, const char *name,
	                      void (*func)(void *), uint32_t stack_size, void *arg);
	int (* thread_delete)(void **thread);
	void (* delay_ms)(uint32_t ms);
} bk_video_osi_funcs_t;

bk_err_t bk_video_osi_funcs_init(void);

