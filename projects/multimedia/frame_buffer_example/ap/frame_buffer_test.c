#include <components/system.h>
#include <os/os.h>
#include <os/str.h>
#include <components/shell_task.h>
#include <stdint.h>

#include <components/bk_frame_buffer.h>
#include <avdk_utils.h>

#define TAG "fb_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)



#define HEAD_SIZE (32)

void frame_buffer_mem_dump_test(void)
{
    uint8_t *frame1, *frame2, *frame3;
    int size = 64;

    LOGI("%s: start\n", __func__);

    bk_mem_slab_dump_heap(MEM_SLAB_HEAP_UNCODED);

    frame1 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    frame2 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    frame3 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);

    if (frame1 == NULL || frame2 == NULL || frame3 == NULL)
    {
        LOGE("%s: malloc failed\n", __func__);
        return;
    }

    for (int i = 0; i < size; i++)
    {
       frame1[i] = 0x11;
       frame2[i] = 0x22;
       frame3[i] = 0x33;
    }

    bk_mem_slab_dump_heap(MEM_SLAB_HEAP_UNCODED);

    LOGI("%s: frame1: %p\n", __func__, frame1);
    avdk_hex_dump((char *)frame1 - HEAD_SIZE, size + HEAD_SIZE * 2, (uint32)((char *)frame1 - HEAD_SIZE));
    LOGI("%s: frame2: %p\n", __func__, frame2);
    avdk_hex_dump((char *)frame2 - HEAD_SIZE, size + HEAD_SIZE * 2, (uint32)((char *)frame2 - HEAD_SIZE));
    LOGI("%s: frame3: %p\n", __func__, frame3);
    avdk_hex_dump((char *)frame3 - HEAD_SIZE, size + HEAD_SIZE * 2, (uint32)((char *)frame3 - HEAD_SIZE));


    bk_frame_buffer_free(frame1);
    bk_frame_buffer_free(frame2);
    bk_frame_buffer_free(frame3);

    LOGI("%s: frame1: %p\n", __func__, frame1);
    avdk_hex_dump((char *)frame1 - HEAD_SIZE, size + HEAD_SIZE * 2, (uint32)((char *)frame1 - HEAD_SIZE));
    LOGI("%s: frame2: %p\n", __func__, frame2);
    avdk_hex_dump((char *)frame2 - HEAD_SIZE, size + HEAD_SIZE * 2, (uint32)((char *)frame2 - HEAD_SIZE));
    LOGI("%s: frame3: %p\n", __func__, frame3);
    avdk_hex_dump((char *)frame3 - HEAD_SIZE, size + HEAD_SIZE * 2, (uint32)((char *)frame3 - HEAD_SIZE));

    bk_mem_slab_dump_heap(MEM_SLAB_HEAP_UNCODED);

    LOGI("%s: end\n", __func__);
}

void frame_buffer_mem_overflow_test1(void)
{
    uint8_t *frame1, *frame2, *frame3, *p;
    int size = 64;


    LOGI("%s: start\n", __func__);

    frame1 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    frame2 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    frame3 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);

    if (frame1 == NULL || frame2 == NULL || frame3 == NULL)
    {
        LOGE("%s: malloc failed\n", __func__);
        return;
    }

    for (int i = 0; i < size; i++)
    {
       frame1[i] = 0x11;
       frame2[i] = 0x22;
       frame3[i] = 0x33;
    }

    p = frame2 + size;
    *p = 0x44;

    LOGI("%s: frame2: %p\n", __func__, frame1);
    avdk_hex_dump((char *)frame2 - HEAD_SIZE - 16, size + HEAD_SIZE + 32, (uint32)((char *)frame2 - HEAD_SIZE - 16));

    bk_frame_buffer_free(frame2);

    LOGI("%s: end\n", __func__);
}

void frame_buffer_mem_overflow_test2(void)
{
    uint8_t *frame1, *frame2, *frame3, *p;
    int size = 64;


    LOGI("%s: start\n", __func__);

    frame1 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    frame2 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    frame3 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);

    if (frame1 == NULL || frame2 == NULL || frame3 == NULL)
    {
        LOGE("%s: malloc failed\n", __func__);
        return;
    }

    for (int i = 0; i < size; i++)
    {
       frame1[i] = 0x11;
       frame2[i] = 0x22;
       frame3[i] = 0x33;
    }

    p = frame2 + size;
    *p = 0x44;

    LOGI("%s: frame2: %p\n", __func__, frame1);
    avdk_hex_dump((char *)frame2 - HEAD_SIZE - 16, size + HEAD_SIZE + 32, (uint32)((char *)frame2 - HEAD_SIZE - 16));

    bk_mem_slab_check_all_heaps();

    LOGI("%s: end\n", __func__);
}

void frame_buffer_test(void)
{
    bk_frame_buffer_init();
    frame_buffer_mem_dump_test();

    //frame_buffer_mem_overflow_test1();

    //frame_buffer_mem_overflow_test2();
}