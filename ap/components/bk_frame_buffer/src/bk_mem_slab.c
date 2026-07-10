//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <common/bk_assert.h>
#include <soc/soc.h>

#include <driver/int.h>

#include <driver/psram.h>
#include "bk_mem_slab.h"

#define TAG "mem_slab"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define FB_LIST_PATTERN           (0xA55AA55A)
#define FB_ALLOCATED_PATTERN      (0x83388338)
#define FB_FREE_PATTERN           (0xF00FF00F)

#define BK_FRAME_BUFFER_SUPPORTED_FLAGS (BK_FRAME_BUFFER_FLAG_WRITE_THROUGH)
#define BK_MEM_SLAB_WRITE_THROUGH_CHANNEL_INVALID (PSRAM_WRITE_THROUGH_AREA_COUNT)

fb_mem_heap_t frame_mem_heap = {0};

#define MEM_SLAB_ERR_OK                     (0)
#define MEM_SLAB_ERR_OVERFLOW_INVALID_DATA  (-2)
#define MEM_SLAB_ERR_OVERFLOW_HEAD_FRONT    (-3)
#define MEM_SLAB_ERR_OVERFLOW_HEAD_BACK     (-4)
#define MEM_SLAB_ERR_OVERFLOW_DATA_TAIL     (-5)
#define MEM_SLAB_ERR_OVERFLOW_FREE_SIZE     (-6)
#define MEM_SLAB_ERR_OVERFLOW_NEXT_POINTER  (-7)
#define MEM_SLAB_ERR_OVERFLOW_PREVIOUS_POINTER (-8)
#define MEM_SLAB_ERR_OVERFLOW_CIRCULAR_REFERENCE (-9)


void bk_mem_slab_heap_init(uint8_t type, uint8_t *heap, uint32_t heap_size)
{
    uint32_t head_address = SLAB_ALIGN_BYTES(((uint32_t)(uintptr_t)heap), ALIGN_BYTES);
    heap_size = heap_size - (head_address - (uint32_t)(uintptr_t)heap);
    heap_size = heap_size & ~((ALIGN_BYTES) - 1);

    // align first free descriptor to word boundary
    frame_mem_heap.heap[type] = (struct fb_block_free *)head_address;

    // initialize the first block
    // compute the size from the last aligned word before heap_end
    frame_mem_heap.heap[type]->free_size = heap_size;
    frame_mem_heap.heap[type]->corrupt_check = FB_LIST_PATTERN;
    frame_mem_heap.heap[type]->next = NULL;
    frame_mem_heap.heap[type]->previous = NULL;
#if MEM_SLAB_MEM_DEBUG
    frame_mem_heap.heap[type]->head_end_check = FB_LIST_PATTERN;
#endif
    frame_mem_heap.heap_size[type] = heap_size;
    LOGD("%s heap:%p, type %d size %d\n", __func__, heap, type, heap_size);
    LOGD("%s, free_size:%d, check:0x%x\r\n", __func__, frame_mem_heap.heap[type]->free_size, frame_mem_heap.heap[type]->corrupt_check);
}

void bk_mem_slab_heap_resume(uint8_t type, uint8_t *heap, uint32_t heap_size)
{
    frame_mem_heap.heap[type] = (struct fb_block_free *)heap;
    frame_mem_heap.heap_size[type] = heap_size;
}

/**
 * Check if memory pointer is within heap address range
 *
 * @param[in] type Memory type.
 * @param[in] mem_ptr Memory pointer
 * @return True if it's in memory heap, False else.
 */
static bool bk_mem_slab_is_in_heap(uint8_t type, void *mem_ptr)
{
    bool ret = false;
    uint8_t *block = (uint8_t *)frame_mem_heap.heap[type];
    uint32_t size = frame_mem_heap.heap_size[type];

    if ((((uint32_t)(uintptr_t)mem_ptr) >= ((uint32_t)(uintptr_t)block))
        && (((uint32_t)(uintptr_t)mem_ptr) <= (((uint32_t)(uintptr_t)block) + size)))
    {
        ret = true;
    }

    return ret;
}

#if (CONFIG_PSRAM_WRITE_THROUGH)
static uint32_t bk_mem_slab_get_payload_size(fb_block_used *block)
{
    return block->size - sizeof(fb_block_used);
}

static uint32_t bk_mem_slab_get_request_size(fb_block_used *block)
{
#if MEM_SLAB_MEM_DEBUG
    return block->user_size;
#else
    return bk_mem_slab_get_payload_size(block);
#endif
}

static bool bk_mem_slab_is_write_through_aligned(uint32_t value)
{
    return ((value & (ALIGN_BYTES - 1)) == 0);
}

static psram_write_through_area_t bk_mem_slab_alloc_write_through_channel(uint32_t start)
{
#if CONFIG_PSRAM_INTERLEAVE
    (void)start;
    return bk_psram_alloc_write_through_channel_with_psram_id(0);
#else
    if ((start >= (uint32_t)SOC_PSRAM1_DATA_BASE)
        && (start < ((uint32_t)SOC_PSRAM1_DATA_BASE + SOC_PSRAM_DATA_SIZE)))
    {
        return bk_psram_alloc_write_through_channel_with_psram_id(1);
    }

    return bk_psram_alloc_write_through_channel_with_psram_id(0);
#endif
}

static bk_err_t bk_mem_slab_enable_write_through(fb_block_used *block, void *mem_ptr)
{
    bk_err_t ret = BK_OK;
    uint32_t start = (uint32_t)(uintptr_t)mem_ptr;
    uint32_t request_size = bk_mem_slab_get_request_size(block);
    uint32_t end = start + bk_mem_slab_get_payload_size(block);
    psram_write_through_area_t area = PSRAM_WRITE_THROUGH_AREA_COUNT;

    if ((block->flag & BK_FRAME_BUFFER_FLAG_WRITE_THROUGH) != 0)
    {
        return BK_OK;
    }

    if (!bk_mem_slab_is_write_through_aligned(start)
        || !bk_mem_slab_is_write_through_aligned(request_size))
    {
        LOGE("%s write-through setup failed, frame:%p addr_align:%u size:%u size_align:%u, require %u-byte aligned addr and size\n",
            __func__, mem_ptr, start & (ALIGN_BYTES - 1), request_size,
            request_size & (ALIGN_BYTES - 1), ALIGN_BYTES);
        return BK_ERR_PARAM;
    }

    area = bk_mem_slab_alloc_write_through_channel(start);
    if (area >= PSRAM_WRITE_THROUGH_AREA_COUNT)
    {
        return BK_ERR_NO_MEM;
    }

    ret = bk_psram_enable_write_through(area, start, end);
    if (ret != BK_OK)
    {
        (void)bk_psram_free_write_through_channel(area);
        return ret;
    }

    block->flag |= BK_FRAME_BUFFER_FLAG_WRITE_THROUGH;
    block->write_through_channel = area;

    return BK_OK;
}

static void bk_mem_slab_cleanup_write_through(fb_block_used *block)
{
    uint32_t area = block->write_through_channel;

    if ((block->flag & BK_FRAME_BUFFER_FLAG_WRITE_THROUGH) == 0)
    {
        return;
    }

    block->flag &= ~BK_FRAME_BUFFER_FLAG_WRITE_THROUGH;
    block->write_through_channel = BK_MEM_SLAB_WRITE_THROUGH_CHANNEL_INVALID;

    if (area >= PSRAM_WRITE_THROUGH_AREA_COUNT)
    {
        LOGW("%s invalid write-through area %u\n", __func__, area);
        return;
    }

    if (bk_psram_disable_write_through((psram_write_through_area_t)area) != BK_OK)
    {
        LOGW("%s disable write-through area %u failed\n", __func__, area);
    }

    if (bk_psram_free_write_through_channel((psram_write_through_area_t)area) != BK_OK)
    {
        LOGW("%s free write-through area %u failed\n", __func__, area);
    }
}
#else
static bk_err_t bk_mem_slab_enable_write_through(fb_block_used *block, void *mem_ptr)
{
    (void)block;
    (void)mem_ptr;

    return BK_ERR_NOT_SUPPORT;
}

static void bk_mem_slab_cleanup_write_through(fb_block_used *block)
{
    (void)block;
}
#endif

void bk_mem_slab_init(void)
{
    os_memset(frame_mem_heap.heap, 0, sizeof(struct fb_block_free *) * MEM_SLAB_HEAP_MAX);
    os_memset(frame_mem_heap.heap_size, 0, sizeof(uint32_t) * MEM_SLAB_HEAP_MAX);
}

#if MEM_SLAB_MEM_DEBUG
void *bk_mem_slab_malloc_debug(frame_buffer_heap_type_t type, uint32_t size, const char *func, uint32_t line)
#else
void *bk_mem_slab_malloc(frame_buffer_heap_type_t type, uint32_t size)
#endif
{
    struct fb_block_free *node = NULL, *found = NULL;
    uint8_t cursor = 0;
    fb_block_used *alloc = NULL;
    uint32_t totalsize, user_size = size;

    if (frame_mem_heap.heap_size[type] == 0)
    {
        LOGE("%s, type:%d not init\r\n", __func__, type);
        BK_ASSERT(0);
    }

    GLOBAL_INT_DECLARATION();

    size = SLAB_ALIGN_BYTES(size, ALIGN_BYTES);

#if MEM_SLAB_MEM_DEBUG
    if (user_size == size)
    {
        size += ALIGN_BYTES;
    }
#endif

    totalsize = size + sizeof(fb_block_used);

    if (totalsize < sizeof(struct fb_block_free))
    {
        totalsize = sizeof(struct fb_block_free);
    }

    // sanity check: the totalsize should be large enough to hold free block descriptor
    BK_ASSERT(totalsize >= sizeof(struct fb_block_free));

    // protect accesses to descriptors
    GLOBAL_INT_DISABLE();

    uint8_t heap_id = COMMON_MOD((cursor + type), MEM_SLAB_HEAP_MAX);

    // Select Heap to use, first try to use current heap.
    node = frame_mem_heap.heap[heap_id];
    BK_ASSERT(node != NULL);

    // go through free memory blocks list
    while (node != NULL)
    {
        BK_ASSERT(node->corrupt_check == FB_LIST_PATTERN);

        // check if there is enough room in this free block
        if (node->free_size >= (totalsize))
        {
            if ((node->free_size >= (totalsize + sizeof(struct fb_block_free)))
                || (node->previous != NULL))
            {
                // if a match was already found, check if this one is smaller
                if ((found == NULL) || (found->free_size > node->free_size))
                {
                    found = node;
                }
            }
        }

        // move to next block
        node = node->next;
    }

    // Update size to use complete list if possible.
    if (found != NULL)
    {
        if (found->free_size < (totalsize + sizeof(struct fb_block_free)))
        {
            totalsize = found->free_size;
        }
    }

    //BT_ASSERT_INFO(found != NULL, size, type);
    // Re-boot platform if no more empty space
    if (found == NULL)
    {
        //platform_reset(RESET_MEM_ALLOC_FAIL);
        GLOBAL_INT_RESTORE();
        return NULL;
    }
    else
    {
        // DBG_MEM_GRANT_CTRL(found, true);
        // sublist completely reused
        if (found->free_size == totalsize)
        {
            BK_ASSERT(found->previous != NULL);
            // update double linked list
            found->previous->next = found->next;
            if (found->next != NULL)
            {
                found->next->previous = found->previous;
            }

            // compute the pointer to the beginning of the free space
            alloc = (fb_block_used *)((uintptr_t)found);
        }
        else
        {
            // found a free block that matches, subtract the allocation size from the
            // free block size. If equal, the free block will be kept with 0 size... but
            // moving it out of the linked list is too much work.
            found->free_size -= totalsize;

            // compute the pointer to the beginning of the free space
            alloc = (fb_block_used *)((uintptr_t)found + found->free_size);
        }

        //TRC_REQ_MEM_ALLOC(trc_heap_id, alloc, size);

        // save the size of the allocated block
        alloc->size = totalsize;
        alloc->corrupt_check = FB_ALLOCATED_PATTERN;
        alloc->flag = 0;
        alloc->write_through_channel = BK_MEM_SLAB_WRITE_THROUGH_CHANNEL_INVALID;
#if MEM_SLAB_MEM_DEBUG
        alloc->func = func;
        alloc->line = line;
        alloc->head_end_check = FB_ALLOCATED_PATTERN;
        alloc->user_size = user_size;

        if (size - user_size > 4)
        {
            uint32_t *padding = (uint32_t*)((uint8_t*)alloc + user_size + sizeof(fb_block_used));
            *padding = FB_ALLOCATED_PATTERN;
            LOGV("padding: %p, %x\n", padding, *padding);
        }
#endif

        // move to the user memory space
        alloc++;
    }

    // end of protection (as early as possible)
    GLOBAL_INT_RESTORE();
    //BK_ASSERT(node == NULL);

    return (void *)alloc;
}

int bk_mem_slab_overflow_check(fb_block_used *head)
{
    uint32_t *padding = (uint32_t*)((uint8_t*)head + sizeof(fb_block_used) + head->user_size);

    LOGV("overflow check: %x, %x, %x\n", head->corrupt_check, head->head_end_check, *padding);

    if (head->corrupt_check != FB_ALLOCATED_PATTERN)
    {
        return MEM_SLAB_ERR_OVERFLOW_HEAD_FRONT;
    }
    if (head->head_end_check != FB_ALLOCATED_PATTERN)
    {
        return MEM_SLAB_ERR_OVERFLOW_HEAD_BACK;
    }

    if (head->size - head->user_size > 4)
    {
        if (*padding != FB_ALLOCATED_PATTERN)
        {
            LOGE("head: %p, %d, %x\n", head, head->user_size, *padding);
            return MEM_SLAB_ERR_OVERFLOW_DATA_TAIL;
        }
    }

    return MEM_SLAB_ERR_OK;
}

bk_err_t bk_mem_slab_set(void *mem_ptr, uint32_t flags)
{
    fb_block_used *block = NULL;
    int ret = MEM_SLAB_ERR_OK;
    GLOBAL_INT_DECLARATION();

    if (mem_ptr == NULL)
    {
        return BK_ERR_NULL_PARAM;
    }

    if ((flags & ~BK_FRAME_BUFFER_SUPPORTED_FLAGS) != 0)
    {
        return BK_ERR_NOT_SUPPORT;
    }

    block = ((fb_block_used *)mem_ptr) - 1;

    GLOBAL_INT_DISABLE();
    ret = bk_mem_slab_overflow_check(block);
    GLOBAL_INT_RESTORE();
    if (ret != MEM_SLAB_ERR_OK)
    {
        LOGE("%s invalid frame buffer: %d\n", __func__, ret);
        return BK_ERR_PARAM;
    }

    if ((flags & BK_FRAME_BUFFER_FLAG_WRITE_THROUGH) != 0)
    {
        return bk_mem_slab_enable_write_through(block, mem_ptr);
    }

    return BK_OK;
}

void bk_mem_slab_free(void *mem_ptr)
{
    struct fb_block_free *freed;
    fb_block_used *bfreed;
    struct fb_block_free *node, *next_node, *prev_node;
    uint32_t size;
    uint8_t cursor = 0;
    int ret = -1;
    GLOBAL_INT_DECLARATION();

    // sanity checks
    if (mem_ptr == NULL)
    {
        return;
    }

    //debug_mem_reset((uint32_t*)mem_ptr);
    // point to the block descriptor (before user memory so decrement)
    bfreed = ((fb_block_used *)mem_ptr) - 1;

    GLOBAL_INT_DISABLE();
    ret = bk_mem_slab_overflow_check(bfreed);
    GLOBAL_INT_RESTORE();

    if (ret != MEM_SLAB_ERR_OK)
    {
        LOGE("frame buffer overflow : %d\n", ret);
        BK_ASSERT(0);
        return;
    }

    bk_mem_slab_cleanup_write_through(bfreed);

    // check if memory block has been corrupted or not
    //BT_ASSERT_INFO(bfreed->corrupt_check == FB_ALLOCATED_PATTERN, bfreed->corrupt_check, mem_ptr);
    // change corruption token in order to know if buffer has been already freed.
    bfreed->corrupt_check = FB_FREE_PATTERN;
#if MEM_SLAB_MEM_DEBUG
    bfreed->head_end_check = FB_FREE_PATTERN;
#endif

    // point to the first node of the free elements linked list
    size = bfreed->size;
    node = NULL;

    freed = ((struct fb_block_free *)bfreed);

    //DBG_MEM_PERM_SET(bfreed, sizeof(struct fb_block_used), false, false, false);

    // protect accesses to descriptors
    GLOBAL_INT_DISABLE();
    //DBG_MEM_GRANT_CTRL(mem_ptr, true);

    // Retrieve where memory block comes from
    while (((cursor < MEM_SLAB_HEAP_MAX)) && (node == NULL))
    {
        if (bk_mem_slab_is_in_heap(cursor, mem_ptr))
        {
            // Select Heap to use, first try to use current heap.
            node = frame_mem_heap.heap[cursor];
        }
        else
        {
            cursor ++;
        }
    }

    // sanity checks
    BK_ASSERT(node != NULL);
    BK_ASSERT(((uint32_t)(uintptr_t)mem_ptr > (uint32_t)(uintptr_t)node));

    //TRC_REQ_MEM_FREE(trc_heap_id, freed, size);
    //DBG_MEM_PERM_SET(freed, size, false, false, false);

    prev_node = NULL;

    while (node != NULL)
    {

#if MEM_SLAB_MEM_DEBUG
        if (node->corrupt_check != FB_LIST_PATTERN
            && node->corrupt_check != FB_FREE_PATTERN
            && node->corrupt_check != FB_ALLOCATED_PATTERN
        )
        {
            LOGE("%s: node:%p, node->corrupt_check:0x%x", __func__, node, node->corrupt_check);
            BK_ASSERT(0);
            return;
        }
#endif

        BK_ASSERT(node->corrupt_check == FB_LIST_PATTERN);
        // check if the freed block is right after the current block
        if ((uint32_t)(uintptr_t)freed == ((uint32_t)(uintptr_t)node + node->free_size))
        {
            // append the freed block to the current one
            node->free_size += size;

            // check if this merge made the link between free blocks
            if (((uint32_t)(uintptr_t) node->next) == (((uint32_t)(uintptr_t)node) + node->free_size))
            {
                next_node = node->next;
                // add the size of the next node to the current node
                node->free_size += next_node->free_size;
                // update the next of the current node
                BK_ASSERT(next_node != NULL);
                node->next = next_node->next;
                // update linked list.
                if (next_node->next != NULL)
                {
                    next_node->next->previous = node;
                }
            }
            goto free_end;
        }
        else if ((uint32_t)(uintptr_t)freed < (uint32_t)(uintptr_t)node)
        {
            // sanity check: can not happen before first node
            BK_ASSERT(prev_node != NULL);

            // update the next pointer of the previous node
            prev_node->next = freed;
            freed->previous = prev_node;

            freed->corrupt_check = FB_LIST_PATTERN;
#if MEM_SLAB_MEM_DEBUG
            freed->head_end_check = FB_LIST_PATTERN;
#endif
            // check if the released node is right before the free block
            if (((uint32_t)(uintptr_t)freed + size) == (uint32_t)(uintptr_t)node)
            {
                // merge the two nodes
                freed->next = node->next;
                if (node->next != NULL)
                {
                    node->next->previous = freed;
                }
                freed->free_size = node->free_size + size;
            }
            else
            {
                // insert the new node
                freed->next = node;
                node->previous = freed;
                freed->free_size = size;
            }
            goto free_end;
        }

        // move to the next free block node
        prev_node = node;
        node = node->next;

    }

    freed->corrupt_check = FB_LIST_PATTERN;
#if MEM_SLAB_MEM_DEBUG
    freed->head_end_check = FB_LIST_PATTERN;
#endif

    BK_ASSERT(prev_node != NULL);

    if (prev_node != NULL)
    {
        // if reached here, freed block is after last free block and not contiguous
        prev_node->next = (struct fb_block_free *)freed;
        freed->next = NULL;
        freed->previous = prev_node;
        freed->free_size = size;
        freed->corrupt_check = FB_LIST_PATTERN;
#if MEM_SLAB_MEM_DEBUG
        freed->head_end_check = FB_LIST_PATTERN;
#endif
    }


free_end:
    // end of protection
    GLOBAL_INT_RESTORE();
}


void bk_mem_slab_dump_heap(uint8_t type)
{
    struct fb_block_free *node = frame_mem_heap.heap[type];
    uint8_t *heap_start = (uint8_t *)frame_mem_heap.heap[type];
    uint32_t heap_size = frame_mem_heap.heap_size[type];
    uint8_t *heap_end = heap_start + heap_size;
    uint8_t *current = heap_start;
    fb_block_used *alloc_block = NULL;
    struct fb_block_free *free_block = NULL;
    uint32_t allocated_count = 0;
    uint32_t total_allocated = 0;

    LOGI("%s: ==========[Heap: %d] Free List Check ==========\n", __func__, type);
    // Check free list
    while (node != NULL)
    {
        if (node->corrupt_check != FB_LIST_PATTERN)
        {
            LOGE("%s: node:%p, node->corrupt_check:0x%x", __func__, node, node->corrupt_check);
            BK_ASSERT(0);
        }
        LOGI("%s: [FREE] size:%u, corrupt_check:0x%x, next:%p, previous:%p, addr:%p\n", __func__,
            node->free_size, node->corrupt_check, (void *)node->next, (void *)node->previous, (void *)node);
        node = node->next;
    }

    LOGI("%s: ==========[Heap: %d] Allocated Blocks Scan ==========\n", __func__, type);
    // Scan entire heap to find allocated blocks
    while (current < heap_end)
    {
        // Safety check: ensure we have enough space for at least fb_block_used structure
        if ((current + sizeof(fb_block_used)) > heap_end)
        {
            LOGW("%s: [WARN] Reached heap end, remaining: %u bytes\n",
                __func__, (uint32_t)(heap_end - current));
            break;
        }

        // Check if current position is an allocated block
        alloc_block = (fb_block_used *)current;

        if (alloc_block->corrupt_check == FB_ALLOCATED_PATTERN)
        {
            // This is an allocated block
            uint32_t block_size = alloc_block->size;

            // Safety check: ensure block_size is valid
            if (block_size < sizeof(fb_block_used) || (current + block_size) > heap_end)
            {
                LOGE("%s: [ERROR] Invalid block_size:%u at addr:%p\n",
                    __func__, block_size, (void *)current);
                break;
            }

            void *user_ptr = (void *)(current + sizeof(fb_block_used));
            uint32_t user_size = block_size - sizeof(fb_block_used);

#if MEM_SLAB_MEM_DEBUG
            LOGI("%s: [ALLOC] func:%s, line:%d, user_ptr:%p, block_addr:%p, block_size:%u, user_size:%u\n",
                __func__, alloc_block->func, alloc_block->line, user_ptr, (void *)current, block_size, user_size);
#else
            LOGI("%s: [ALLOC] user_ptr:%p, block_addr:%p, block_size:%u, user_size:%u\n",
                __func__, user_ptr, (void *)current, block_size, user_size);
#endif
            allocated_count++;
            total_allocated += block_size;

            // Move to next block
            current += block_size;
        }
        else if (alloc_block->corrupt_check == FB_LIST_PATTERN)
        {
            // This is a free block (in free list)
            // Safety check: ensure we have enough space for fb_block_free structure
            if ((current + sizeof(struct fb_block_free)) > heap_end)
            {
                LOGW("%s: [WARN] Free block structure exceeds heap end\n", __func__);
                break;
            }

            free_block = (struct fb_block_free *)current;
            uint32_t free_size = free_block->free_size;

            // Safety check: ensure free_size is valid
            if (free_size < sizeof(struct fb_block_free) || (current + free_size) > heap_end)
            {
                LOGE("%s: [ERROR] Invalid free_size:%u at addr:%p\n",
                    __func__, free_size, (void *)current);
                break;
            }

            current += free_size;
        }
        else if (alloc_block->corrupt_check == FB_FREE_PATTERN)
        {
            // This block is being freed (transitional state)
            // Use size from fb_block_used structure
            uint32_t block_size = alloc_block->size;

            // Safety check
            if (block_size < sizeof(fb_block_used) || (current + block_size) > heap_end)
            {
                LOGE("%s: [ERROR] Invalid freeing block_size:%u at addr:%p\n",
                    __func__, block_size, (void *)current);
                break;
            }

            LOGI("%s: [FREEING] addr:%p, size:%u\n", __func__, (void *)current, block_size);
            current += block_size;
        }
        else
        {
            // Unknown pattern, try to skip by minimum block size
            LOGW("%s: [UNKNOWN] addr:%p, corrupt_check:0x%x, skip by min size\n",
                __func__, (void *)current, alloc_block->corrupt_check);
            current += sizeof(fb_block_used);

            // Safety check to prevent infinite loop
            if (current >= heap_end)
            {
                break;
            }
        }
    }

    LOGI("%s: ==========[Heap: %d] Summary ==========\n", __func__, type);
    LOGI("%s: Allocated blocks count: %u, Total allocated size: %u bytes\n",
        __func__, allocated_count, total_allocated);
    LOGI("%s: Heap size: %u bytes, Heap start: %p, Heap end: %p\n",
        __func__, heap_size, (void *)heap_start, (void *)heap_end);
}

void bk_mem_slab_dump_all_heaps(void)
{
    for (uint8_t i = 0; i < MEM_SLAB_HEAP_MAX; i++)
    {
        bk_mem_slab_dump_heap(i);
    }
}

void bk_mem_slab_check_heap(uint8_t type)
{
    struct fb_block_free *node = NULL;
    uint8_t *heap_start = NULL;
    uint32_t heap_size = 0;
    uint8_t *heap_end = NULL;
    bool error_found = false;
    uint32_t visited_count = 0;
    uint32_t max_visited = 1000; // Prevent infinite loop
    void *error_node = NULL;
    void *error_ptr = NULL;
    uint32_t error_value = 0;
    uint32_t error_expected = 0;
    uint32_t error_size = 0;
    int error_type = 0; // 0: no error, 1: free node out of range, 2: corrupt_check, 3: free_size, 4: next/previous pointer
    bool is_allocated_block_error = false; // Flag to indicate if error is from allocated block
    GLOBAL_INT_DECLARATION();

    // Check if heap is initialized
    if (frame_mem_heap.heap_size[type] == 0)
    {
        LOGE("%s: type:%d heap not initialized\r\n", __func__, type);
        return;
    }

    if (frame_mem_heap.heap[type] == NULL)
    {
        LOGE("%s: type:%d heap pointer is NULL\r\n", __func__, type);
        return;
    }

    heap_start = (uint8_t *)frame_mem_heap.heap[type];
    heap_size = frame_mem_heap.heap_size[type];
    heap_end = heap_start + heap_size;

    LOGI("%s: ==========[Heap: %d] Corruption Check Start ==========\n", __func__, type);

    // Protect heap access to prevent other threads or interrupt handlers from modifying heap structure during check
    GLOBAL_INT_DISABLE();

    // 1. Check free list
    node = frame_mem_heap.heap[type];
    visited_count = 0;

    while (node != NULL && visited_count < max_visited && !error_found)
    {
        visited_count++;

        // Check if node is within heap range
        if (!bk_mem_slab_is_in_heap(type, node))
        {
            error_found = true;
            error_type = MEM_SLAB_ERR_OVERFLOW_INVALID_DATA;
            error_node = (void *)node;
            break;
        }

        // Check corrupt_check field
        if (node->corrupt_check != FB_LIST_PATTERN)
        {
            error_found = true;
            error_type = MEM_SLAB_ERR_OVERFLOW_HEAD_FRONT;
            error_node = (void *)node;
            error_value = node->corrupt_check;
            error_expected = FB_LIST_PATTERN;
            break;
        }

        if (node->head_end_check != FB_LIST_PATTERN)
        {
            error_found = true;
            error_type = MEM_SLAB_ERR_OVERFLOW_HEAD_BACK;
            error_node = (void *)node;
            error_value = node->head_end_check;
            error_expected = FB_LIST_PATTERN;
            break;
        }

        // Check if free_size is reasonable
        if (node->free_size < sizeof(struct fb_block_free))
        {
            error_found = true;
            error_type = MEM_SLAB_ERR_OVERFLOW_FREE_SIZE;
            error_node = (void *)node;
            error_size = node->free_size;
            break;
        }

        // Check if free_size exceeds heap range
        if ((uint8_t *)node + node->free_size > heap_end)
        {
            error_found = true;
            error_type = MEM_SLAB_ERR_OVERFLOW_FREE_SIZE;
            error_node = (void *)node;
            error_size = node->free_size;
            break;
        }

        // Check next pointer
        if (node->next != NULL)
        {
            if (!bk_mem_slab_is_in_heap(type, node->next))
            {
                error_found = true;
                error_type = MEM_SLAB_ERR_OVERFLOW_NEXT_POINTER;
                error_node = (void *)node;
                error_ptr = (void *)node->next;
                break;
            }
            else if (node->next->previous != node)
            {
                error_found = true;
                error_type = MEM_SLAB_ERR_OVERFLOW_PREVIOUS_POINTER;
                error_node = (void *)node;
                error_ptr = (void *)node->next->previous;
                break;
            }
        }

        // Check previous pointer
        if (node->previous != NULL)
        {
            if (!bk_mem_slab_is_in_heap(type, node->previous))
            {
                error_found = true;
                error_type = MEM_SLAB_ERR_OVERFLOW_PREVIOUS_POINTER;
                error_node = (void *)node;
                error_ptr = (void *)node->previous;
                break;
            }
            else if (node->previous->next != node)
            {
                error_found = true;
                error_type = MEM_SLAB_ERR_OVERFLOW_NEXT_POINTER;
                error_node = (void *)node;
                error_ptr = (void *)node->previous->next;
                break;
            }
        }

        node = node->next;
    }

    if (!error_found && visited_count >= max_visited)
    {
        error_found = true;
        error_type = MEM_SLAB_ERR_OVERFLOW_CIRCULAR_REFERENCE;
    }

    // 2. Check if allocated blocks (fb_block_used) are corrupted
    if (!error_found)
    {
        uint8_t *current = heap_start;
        fb_block_used *alloc_block = NULL;

        while (current < heap_end && !error_found)
        {
            // Safety check: ensure there is enough space for at least fb_block_used structure
            if ((current + sizeof(fb_block_used)) > heap_end)
            {
                break;
            }

            alloc_block = (fb_block_used *)current;

            // Check allocated block
            if (alloc_block->corrupt_check == FB_ALLOCATED_PATTERN)
            {
                uint32_t block_size = alloc_block->size;

                // Check if block size is reasonable
                if (block_size < sizeof(fb_block_used) || (current + block_size) > heap_end)
                {
                    error_found = true;
                    error_type = MEM_SLAB_ERR_OVERFLOW_INVALID_DATA;
                    error_node = (void *)current;
                    error_size = block_size;
                    is_allocated_block_error = true;
                    break;
                }

                // corrupt_check is already checked in if condition, check other fields here

#if MEM_SLAB_MEM_DEBUG
                // Check head_end_check field
                if (alloc_block->head_end_check != FB_ALLOCATED_PATTERN)
                {
                    error_found = true;
                    error_type = MEM_SLAB_ERR_OVERFLOW_HEAD_BACK;
                    error_node = (void *)current;
                    error_value = alloc_block->head_end_check;
                    error_expected = FB_ALLOCATED_PATTERN;
                    is_allocated_block_error = true;
                    break;
                }

                // Check padding area (if exists)
                if (alloc_block->size - alloc_block->user_size > 4)
                {
                    uint32_t *padding = (uint32_t*)((uint8_t*)alloc_block + sizeof(fb_block_used) + alloc_block->user_size);
                    if (*padding != FB_ALLOCATED_PATTERN)
                    {
                        error_found = true;
                        error_type = MEM_SLAB_ERR_OVERFLOW_DATA_TAIL;
                        error_node = (void *)current;
                        error_value = *padding;
                        error_expected = FB_ALLOCATED_PATTERN;
                        is_allocated_block_error = true;
                        break;
                    }
                }
#endif

                current += block_size;
            }
            // Skip free block (in free list)
            else if (alloc_block->corrupt_check == FB_LIST_PATTERN)
            {
                if ((current + sizeof(struct fb_block_free)) > heap_end)
                {
                    break;
                }

                struct fb_block_free *free_block = (struct fb_block_free *)current;
                uint32_t free_size = free_block->free_size;

                if (free_size < sizeof(struct fb_block_free) || (current + free_size) > heap_end)
                {
                    break;
                }

                current += free_size;
            }
            // Skip block being freed
            else if (alloc_block->corrupt_check == FB_FREE_PATTERN)
            {
                uint32_t block_size = alloc_block->size;

                if (block_size < sizeof(fb_block_used) || (current + block_size) > heap_end)
                {
                    break;
                }

                current += block_size;
            }
            else
            {
                // Unknown pattern, memory may be corrupted
                error_found = true;
                error_type = MEM_SLAB_ERR_OVERFLOW_HEAD_FRONT;
                error_node = (void *)current;
                error_value = alloc_block->corrupt_check;
                error_expected = 0; // 0 means unknown, could be FB_ALLOCATED_PATTERN, FB_LIST_PATTERN or FB_FREE_PATTERN
                is_allocated_block_error = true;
                break;
            }
        }
    }

    // Restore interrupt protection
    GLOBAL_INT_RESTORE();

    // Print error information
    if (error_found)
    {
        void *buffer_addr = NULL;
        const char *error_desc = NULL;

        if (error_node != NULL && error_type != MEM_SLAB_ERR_OVERFLOW_CIRCULAR_REFERENCE)
        {
            if (is_allocated_block_error)
            {
                // Buffer address of allocated block
                buffer_addr = (uint8_t *)error_node + sizeof(fb_block_used);
            }
            else
            {
                // Buffer address of free block
                buffer_addr = (uint8_t *)error_node + sizeof(struct fb_block_free);
            }
        }

        // Get error description based on error type
        switch (error_type)
        {
            case MEM_SLAB_ERR_OVERFLOW_INVALID_DATA:
                if (is_allocated_block_error)
                {
                    error_desc = "Allocated block size invalid or out of heap range";
                }
                else
                {
                    error_desc = "Free node out of heap range";
                }
                break;
            case MEM_SLAB_ERR_OVERFLOW_HEAD_FRONT:
                if (is_allocated_block_error)
                {
                    if (error_expected == 0)
                    {
                        error_desc = "Allocated block unknown pattern (memory corrupted)";
                    }
                    else
                    {
                        error_desc = "Allocated block corrupt_check mismatch";
                    }
                }
                else
                {
                    error_desc = "Free node corrupt_check mismatch";
                }
                break;
            case MEM_SLAB_ERR_OVERFLOW_HEAD_BACK:
                if (is_allocated_block_error)
                {
                    error_desc = "Allocated block head_end_check mismatch";
                }
                else
                {
                    error_desc = "Free node head_end_check mismatch";
                }
                break;
            case MEM_SLAB_ERR_OVERFLOW_DATA_TAIL:
                if (is_allocated_block_error)
                {
                    error_desc = "Allocated block data tail padding corrupted";
                }
                else
                {
                    error_desc = "Free block data tail padding corrupted";
                }
                break;
            case MEM_SLAB_ERR_OVERFLOW_FREE_SIZE:
                error_desc = "free_size invalid";
                break;
            case MEM_SLAB_ERR_OVERFLOW_NEXT_POINTER:
                error_desc = "next pointer invalid";
                break;
            case MEM_SLAB_ERR_OVERFLOW_PREVIOUS_POINTER:
                error_desc = "previous pointer invalid";
                break;
            case MEM_SLAB_ERR_OVERFLOW_CIRCULAR_REFERENCE:
                error_desc = "circular reference or list too long";
                break;
            default:
                error_desc = "unknown error";
                break;
        }

        LOGE("%s: ==========[Heap: %d] Corruption Check FAILED ==========\n", __func__, type);
        LOGE("%s: [ERROR] Error type: %d (%s)\n", __func__, error_type, error_desc);
        if (error_node != NULL)
        {
            if (is_allocated_block_error)
            {
                LOGE("%s: [ERROR] Allocated block: %p, buffer_addr: %p\n", __func__, error_node, buffer_addr);
            }
            else
            {
                LOGE("%s: [ERROR] Free node: %p, buffer_addr: %p\n", __func__, error_node, buffer_addr);
            }
        }
        if (error_value != 0 || error_expected != 0)
        {
            if (error_expected == 0)
            {
                LOGE("%s: [ERROR] Unknown pattern: 0x%x (expected: FB_ALLOCATED_PATTERN/0x%x, FB_LIST_PATTERN/0x%x, or FB_FREE_PATTERN/0x%x)\n",
                    __func__, error_value, FB_ALLOCATED_PATTERN, FB_LIST_PATTERN, FB_FREE_PATTERN);
            }
            else
            {
                LOGE("%s: [ERROR] Value: 0x%x, Expected: 0x%x\n", __func__, error_value, error_expected);
            }
        }
        if (error_size != 0)
        {
            LOGE("%s: [ERROR] Size: %u (min: %u)\n", __func__, error_size, sizeof(struct fb_block_free));
        }
        if (error_ptr != NULL)
        {
            LOGE("%s: [ERROR] Invalid pointer: %p\n", __func__, error_ptr);
        }
        LOGE("%s: [ERROR] Heap buffer: start:%p end:%p size:%u\n",
            __func__, heap_start, heap_end, heap_size);
#if MEM_SLAB_MEM_DEBUG
        BK_ASSERT(0);
#endif
        return;
    }

    LOGI("%s: ==========[Heap: %d] Corruption Check PASSED ==========\n", __func__, type);
}

void bk_mem_slab_check_all_heaps(void)
{
    for (uint8_t i = 0; i < MEM_SLAB_HEAP_MAX; i++)
    {
        bk_mem_slab_check_heap(i);
    }
}