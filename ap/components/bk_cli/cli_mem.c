#include <stdlib.h>
#include "cli.h"
#include <os/os.h>
#include <os/mem.h>
#include <components/system.h>
#include "bk_rtos_debug.h"
#if CONFIG_PSRAM
#include <driver/psram.h>
#endif

/* Platform includes. */
#include "sys_driver.h"
#include <soc/soc.h>

#if CONFIG_AON_RTC
#include <driver/aon_rtc.h>
#include <driver/aon_rtc_types.h>
#endif

void cli_memory_free_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t total_size,free_size,mini_size;
    CLI_LOGD("================Static memory================\r\n");
    os_show_memory_config_info();

	CLI_LOGD("================Dynamic memory================\r\n");
	cmd_printf("%-5s   %-5s   %-5s   %-5s   %-5s\r\n",
		"name", "total", "free", "minimum", "peak");
	
	total_size = rtos_get_total_heap_size();
	free_size  = rtos_get_free_heap_size();
	mini_size  = rtos_get_minimum_free_heap_size();
	cmd_printf("heap\t%d\t%d\t%d\t%d\r\n",  total_size,free_size,mini_size,total_size-mini_size);

#if CONFIG_PSRAM_AS_SYS_MEMORY
	total_size = rtos_get_psram_total_heap_size();
	free_size  = rtos_get_psram_free_heap_size();
	mini_size  = rtos_get_psram_minimum_free_heap_size();
	cmd_printf("psram\t%d\t%d\t%d\t%d\r\n", total_size,free_size,mini_size,total_size-mini_size);
#endif

#if CONFIG_AP_HSRAM_HEAP_ADDR
	total_size = rtos_get_hsram_total_heap_size();
	free_size  = rtos_get_hsram_free_heap_size();
	mini_size  = rtos_get_hsram_minimum_free_heap_size();
	cmd_printf("hsram\t%d\t%d\t%d\t%d\r\n", total_size,free_size,mini_size,total_size-mini_size);
#endif

}

void cli_memory_set_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_DEBUG_VERSION
    uint32_t address, value;
    BK_LOGD(NULL, "cli_memory_set_cmd\r\n");
    if (argc >= 3) {
        address = strtoll(argv[1], NULL, 16);
        value = strtoll(argv[2], NULL, 16);
        BK_LOGD(NULL, "memset,address: 0x%08X value: 0x%08X\r\n", address, value);

        os_write_word(address, value);
    } else {
        BK_LOGD(NULL, "memset <addr> <value>\r\n");
    }
#endif
}

/**
 * @brief Set memory region to a specific pattern value
 * @param addr Memory start address
 * @param size Memory size in bytes
 * @param pattern Pattern value (default 0x5a)
 */
static void mem_set_pattern(uint32_t addr, uint32_t size, uint8_t pattern)
{
    uint8_t *p = (uint8_t *)addr;
    uint32_t i;

    for (i = 0; i < size; i++) {
        p[i] = pattern;
    }
}

/**
 * @brief Set memory region to its own address value
 * @param addr Memory start address
 * @param size Memory size in bytes
 */
static void mem_set_addr(uint32_t addr, uint32_t size)
{
    uint32_t *p = (uint32_t *)addr;
    uint32_t count = size / sizeof(uint32_t);
    uint32_t i;

    for (i = 0; i < count; i++) {
        p[i] = addr + (i * sizeof(uint32_t));
    }

    // Handle remaining bytes if size is not aligned to 4 bytes
    if (size % sizeof(uint32_t) != 0) {
        uint8_t *p_byte = (uint8_t *)(addr + count * sizeof(uint32_t));
        uint32_t remaining = size % sizeof(uint32_t);
        uint32_t remaining_addr = addr + count * sizeof(uint32_t);
        for (i = 0; i < remaining; i++) {
            p_byte[i] = (uint8_t)((remaining_addr + i) & 0xFF);
        }
    }
}

/**
 * @brief Check if memory region contains specific pattern
 * @param addr Memory start address
 * @param size Memory size in bytes
 * @param pattern Pattern value to check (default 0x5a)
 * @return 0 if all match, -1 if mismatch found
 */
static int32_t mem_check_pattern(uint32_t addr, uint32_t size, uint8_t pattern)
{
    uint8_t *p = (uint8_t *)addr;
    uint32_t i;

    for (i = 0; i < size; i++) {
        if (p[i] != pattern) {
            BK_LOGD(NULL, "Pattern check fail @ 0x%08X: expected 0x%02X, got 0x%02X\r\n",
                    addr + i, pattern, p[i]);
            return -1;
        }
    }

    BK_LOGD(NULL, "Pattern check pass: all bytes are 0x%02X\r\n", pattern);
    return 0;
}

/**
 * @brief Check if memory region contains its own address values
 * @param addr Memory start address
 * @param size Memory size in bytes
 * @return 0 if all match, -1 if mismatch found
 */
static int32_t mem_check_addr(uint32_t addr, uint32_t size)
{
    uint32_t *p = (uint32_t *)addr;
    uint32_t count = size / sizeof(uint32_t);
    uint32_t i;

    for (i = 0; i < count; i++) {
        uint32_t expected = addr + (i * sizeof(uint32_t));
        if (p[i] != expected) {
            BK_LOGD(NULL, "Address check fail @ 0x%08X: expected 0x%08X, got 0x%08X\r\n",
                    addr + (i * sizeof(uint32_t)), expected, p[i]);
            return -1;
        }
    }

    // Check remaining bytes if size is not aligned to 4 bytes
    if (size % sizeof(uint32_t) != 0) {
        uint8_t *p_byte = (uint8_t *)(addr + count * sizeof(uint32_t));
        uint32_t remaining = size % sizeof(uint32_t);
        uint32_t remaining_addr = addr + count * sizeof(uint32_t);
        for (i = 0; i < remaining; i++) {
            uint8_t expected = (uint8_t)((remaining_addr + i) & 0xFF);
            if (p_byte[i] != expected) {
                BK_LOGD(NULL, "Address check fail @ 0x%08X: expected 0x%02X, got 0x%02X\r\n",
                        remaining_addr + i, expected, p_byte[i]);
                return -1;
            }
        }
    }

    BK_LOGD(NULL, "Address check pass: all values match their addresses\r\n");
    return 0;
}

static void cli_memset_pattern_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t address, size;
    uint8_t pattern = 0x5a;

    if (argc >= 3) {
        address = strtoll(argv[1], NULL, 16);
        size = strtoll(argv[2], NULL, 16);
        if (argc >= 4) {
            pattern = (uint8_t)strtoll(argv[3], NULL, 16);
        }
        BK_LOGD(NULL, "memset_pattern, address: 0x%08X size: 0x%08X pattern: 0x%02X\r\n",
                address, size, pattern);
        mem_set_pattern(address, size, pattern);
        BK_LOGD(NULL, "memset_pattern done\r\n");
    } else {
        BK_LOGD(NULL, "memset_pattern <addr> <size> [pattern]\r\n");
        BK_LOGD(NULL, "  Set memory region to pattern value (default 0x5a)\r\n");
        BK_LOGD(NULL, "  example: memset_pattern 0x281c0000 0x1000 0x5a\r\n");
    }
}

static void cli_memset_addr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t address, size;

    if (argc >= 3) {
        address = strtoll(argv[1], NULL, 16);
        size = strtoll(argv[2], NULL, 16);
        BK_LOGD(NULL, "memset_addr, address: 0x%08X size: 0x%08X\r\n", address, size);
        mem_set_addr(address, size);
        BK_LOGD(NULL, "memset_addr done\r\n");
    } else {
        BK_LOGD(NULL, "memset_addr <addr> <size>\r\n");
        BK_LOGD(NULL, "  Set memory region to its own address values\r\n");
        BK_LOGD(NULL, "  example: memset_addr 0x281c0000 0x1000\r\n");
    }
}

static void cli_memcheck_pattern_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t address, size;
    uint8_t pattern = 0x5a;
    int32_t result;

    if (argc >= 3) {
        address = strtoll(argv[1], NULL, 16);
        size = strtoll(argv[2], NULL, 16);
        if (argc >= 4) {
            pattern = (uint8_t)strtoll(argv[3], NULL, 16);
        }
        BK_LOGD(NULL, "memcheck_pattern, address: 0x%08X size: 0x%08X pattern: 0x%02X\r\n",
                address, size, pattern);
        result = mem_check_pattern(address, size, pattern);
        if (result == 0) {
            BK_LOGD(NULL, "memcheck_pattern PASS\r\n");
        } else {
            BK_LOGD(NULL, "memcheck_pattern FAIL\r\n");
        }
    } else {
        BK_LOGD(NULL, "memcheck_pattern <addr> <size> [pattern]\r\n");
        BK_LOGD(NULL, "  Check if memory region contains pattern value (default 0x5a)\r\n");
        BK_LOGD(NULL, "  example: memcheck_pattern 0x281c0000 0x1000 0x5a\r\n");
    }
}

static void cli_memcheck_addr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t address, size;
    int32_t result;

    if (argc >= 3) {
        address = strtoll(argv[1], NULL, 16);
        size = strtoll(argv[2], NULL, 16);
        BK_LOGD(NULL, "memcheck_addr, address: 0x%08X size: 0x%08X\r\n", address, size);
        result = mem_check_addr(address, size);
        if (result == 0) {
            BK_LOGD(NULL, "memcheck_addr PASS\r\n");
        } else {
            BK_LOGD(NULL, "memcheck_addr FAIL\r\n");
        }
    } else {
        BK_LOGD(NULL, "memcheck_addr <addr> <size>\r\n");
        BK_LOGD(NULL, "  Check if memory region contains its own address values\r\n");
        BK_LOGD(NULL, "  example: memcheck_addr 0x281c0000 0x1000\r\n");
    }
}

#if CONFIG_DEBUG_VERSION
const static uint32_t s_test_data[20] = {
    0x00000000, 0x800102a0, 0x30021e44, 0x30034088,
    0x5f696c63, 0x61727370, 0x616d5f6d, 0x636f6c6c,
    0x646d635f, 0x00000000, 0x736d656d, 0x6b636174,
    0x00696c63, 0x00000000, 0x00001ed9, 0x0000005c,
    0x00010240, 0x12345678, 0x99887766, 0xaabbccdd
};

__attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns"))) \
int32_t memtest_write_one_word(uint32_t addr, uint32_t count) {
    uint32_t i;
    uint32_t src_data = 0x30023456;
    uint32_t *p_uint32_dst = (uint32_t *)addr;
    for(i = 0; i < count; i++)
    {
        os_write_word(p_uint32_dst, src_data);
        src_data++;
    }
    return 0;
}


__attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns"))) \
int32_t memtest_wr(uint32_t addr, uint32_t count)
{
#if CONFIG_DEBUG_VERSION
    int int_status = rtos_enter_critical();
    BK_LOGD(NULL, "memtest_wr begin!!\r\n");
    os_memcpy_word((uint32_t *)addr, &s_test_data[2], sizeof(s_test_data) - 8);

    bk_mem_dump("before test", addr - 8, sizeof(s_test_data));

    memtest_write_one_word(addr, count);

    bk_mem_dump("after test", addr - 8, sizeof(s_test_data));

    BK_LOGD(NULL, "memtest_wr done!!\r\n");
    rtos_exit_critical(int_status);
#endif //#if CONFIG_DEBUG_VERSION

    return 0;
}


static void cli_memtest_wr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t address, count;

    if (argc >= 3) {
        address = strtoll(argv[1], NULL, 16);
        count = strtoll(argv[2], NULL, 16);
        BK_LOGD(NULL, "memtest_wr,address: 0x%08X count: 0x%08X\r\n", address, count);

        (void)memtest_wr(address, count);
    }  else {
        BK_LOGD(NULL, "memtest_wr <addr> <count> \r\n");
    }
}

#endif //#if CONFIG_DEBUG_VERSION

void cli_memory_stack_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_FREERTOS
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	rtos_dump_stack_memory_usage();
	GLOBAL_INT_RESTORE();
#endif
}

#if CONFIG_MEM_DEBUG && CONFIG_FREERTOS
static void cli_memory_leak_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t start_tick = 0;
	uint32_t ticks_since_malloc = 0;
	uint32_t seconds_since_malloc = 0;
	char *task_name = NULL;

	if (argc >= 2) {
		seconds_since_malloc = os_strtoul(argv[1], NULL, 10);
		ticks_since_malloc = bk_get_ticks_per_second() * seconds_since_malloc;
	}

	if (argc >= 3)
		start_tick = os_strtoul(argv[2], NULL, 10);

	if (argc >= 4)
		task_name = argv[3];

	os_dump_memory_stats(start_tick, ticks_since_malloc, task_name);
}
#endif

#if CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS
void cli_psram_malloc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t *pstart;
	uint32_t length;

	if (argc != 2) {
		cmd_printf("Usage: psram_malloc <length>.\r\n");
		return;
	}

	length = strtoul(argv[1], NULL, 0);

	pstart = (uint8_t *)psram_malloc(length);

	cmd_printf("psram_malloc ret(%p).\r\n", pstart);
}

void cli_psram_free_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t *pstart;
	uint32_t start;

	if (argc != 2) {
		cmd_printf("Usage: psram_free <addr>.\r\n");
		return;
	}

	start = strtoul(argv[1], NULL, 0);
	pstart = (uint8_t *)start;
	cmd_printf("psram_free addr(%p).\r\n", pstart);
	os_free(pstart);

}

void cli_psram_state_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_psram_heap_get_used_state();
}


#endif //#if CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS

int32_t mem_test(uint32_t address, uint32_t size, uint8_t quiet_mode)
{
    uint32_t i;

    /**< 32bit test */
    {
        uint32_t * p_uint32_t = (uint32_t *)address;
        for(i=0; i<size/sizeof(uint32_t); i++)
        {
            *p_uint32_t++ = (uint32_t)i;
        }

        p_uint32_t = (uint32_t *)address;
        for(i=0; i<size/sizeof(uint32_t); i++)
        {
            if( *p_uint32_t != (uint32_t)i )
            {
                if (quiet_mode == 0) {
                    BK_LOGD(NULL, "32bit test fail @ 0x%08X\r\n",(uint32_t)p_uint32_t);
                    return -1;
                }
                while(1);
            }
            p_uint32_t++;
        }

        if (quiet_mode == 0) {
            BK_LOGD(NULL, "32bit test pass!!\r\n");
        }

    }

    /**< 32bit Loopback test */
    {
        uint32_t * p_uint32_t = (uint32_t *)address;
        for(i=0; i<size/sizeof(uint32_t); i++)
        {
            *p_uint32_t  = (uint32_t)p_uint32_t;
            p_uint32_t++;
        }

        p_uint32_t = (uint32_t *)address;
        for(i=0; i<size/sizeof(uint32_t); i++)
        {
            if( *p_uint32_t != (uint32_t)p_uint32_t )
            {
                if (quiet_mode == 0) {
                    BK_LOGD(NULL, "32bit Loopback test fail @ 0x%08X\r\n", (uint32_t)p_uint32_t);
                    BK_LOGD(NULL, " data:0x%08X \r\n", (uint32_t)*p_uint32_t);
                    return -1;
                }

                while(1);
            }
            p_uint32_t++;
        }

        if (quiet_mode == 0) {
            BK_LOGD(NULL, "32bit Loopback test pass!!\r\n");
        }
    }


    /**< 16bit test */
    {
        uint16_t * p_uint16_t = (uint16_t *)address;
        for(i=0; i<size/sizeof(uint16_t); i++)
        {
            *p_uint16_t++ = (uint16_t)i;
        }

        p_uint16_t = (uint16_t *)address;
        for(i=0; i<size/sizeof(uint16_t); i++)
        {
            if( *p_uint16_t != (uint16_t)i )
            {
                if (quiet_mode == 0) {
                    BK_LOGD(NULL, "16bit test fail @ 0x%08X\r\n",(uint32_t)p_uint16_t);
                    return -1;
                }

                while(1);
            }
            p_uint16_t++;
        }

        if (quiet_mode == 0) {
            BK_LOGD(NULL, "16bit test pass!!\r\n");
        }

    }


    /**< 8bit test */
    {
        uint8_t * p_uint8_t = (uint8_t *)address;
        for(i=0; i<size/sizeof(uint8_t); i++)
        {
            *p_uint8_t++ = (uint8_t)i;
        }

        p_uint8_t = (uint8_t *)address;
        for(i=0; i<size/sizeof(uint8_t); i++)
        {
            if( *p_uint8_t != (uint8_t)i )
            {
                if (quiet_mode == 0) {
                    BK_LOGD(NULL, "8bit test fail @ 0x%08X\r\n",(uint32_t)p_uint8_t);
                    return -1;
                }
                while(1);
            }
            p_uint8_t++;
        }
        if (quiet_mode == 0) {
            BK_LOGD(NULL, "8bit test pass!!\r\n");
        }
    }

    return 0;
}

__attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns"))) \
void mem_time(uint32_t *base, uint32_t count, uint8_t mode)
{
    /** write addr x times*/
__maybe_unused volatile uint32_t data = 0;
    volatile uint32_t *address = base;
    uint64_t saved_aon_time = 0, cur_aon_time = 0, diff_time = 0;
    uint32_t diff_ms = 0;

    uint32_t intbk = rtos_enter_critical();

#if CONFIG_AON_RTC
    saved_aon_time = bk_aon_rtc_get_us();
#endif

    switch(mode)
    {
        case 0://single read
            for(uint32_t i=0; i<count; i++)
            {
                data = REG_READ(address);
            }
            break;
        case 1://single write
            for(uint32_t i=0; i<count; i++)
            {
                REG_WRITE(address, i);
            }
            break;
        case 2://read&write
            for(uint32_t i=0; i<count; i++)
            {
                REG_WRITE(address, i);
                data = REG_READ(address);
            }
            break;
        case 3://multi-mem read
            for(uint32_t i=0; i<count; i++)
            {
                data = REG_READ(address);
                address++;
                if(address == (base+256) ) address = base;
            }
            break;
        case 4://multi-mem write
            for(uint32_t i=0; i<count; i++)
            {
                REG_WRITE(address, i);
                address++;
                if(address == (base+256) ) address = base;
            }
        case 5://multi-mem read&write
            for(uint32_t i=0; i<count; i++)
            {
                data = REG_READ(address);
                REG_WRITE(address, i);
                address++;
                if(address == (base+256) ) address = base;
            }
            break;

        default:
            BK_LOGD(NULL, "error!!\r\n");
    }

#if CONFIG_AON_RTC
    cur_aon_time = bk_aon_rtc_get_us();
    diff_time = (cur_aon_time - saved_aon_time);
    diff_ms = (uint32_t)diff_time/1000;
#endif

    rtos_exit_critical(intbk);

    BK_DUMP_OUT("saved time: 0x%x:0x%08x\r\n", (u32)(saved_aon_time >> 32), (u32)(saved_aon_time & 0xFFFFFFFF));
    BK_DUMP_OUT("curr time: 0x%x:0x%08x\r\n", (u32)(cur_aon_time >> 32), (u32)(cur_aon_time & 0xFFFFFFFF));
    BK_DUMP_OUT("diff time: 0x%x:0x%08x\r\n", (u32)(diff_time >> 32), (u32)(diff_time & 0xFFFFFFFF));

    BK_DUMP_OUT("memtime end, time consume=%d ms\r\n", diff_ms);
}

int32_t mem_read_test(uint32_t src, uint32_t dst, uint32_t size)
{
#if CONFIG_DEBUG_VERSION
    uint32_t i;
    uint32_t test_count = size/sizeof(uint32_t);

    os_memcpy_word((uint32_t *)dst, (uint32_t *)src, size);

    /**< 32bit test */
    {
        uint32_t *p_uint32_src = (uint32_t *)src;
        uint32_t *p_uint32_dst = (uint32_t *)dst;

        for(i = 0; i < test_count; i++)
        {
            if( *p_uint32_src != *p_uint32_dst)
            {
                BK_LOGD(NULL, "32bit test fail @ 0x%08X\r\n!",(uint32_t)p_uint32_src);
                return -1;
            }
            p_uint32_src++;
            p_uint32_dst++;
        }

        BK_LOGD(NULL, "32bit test pass!!\r\n");
    }

    /**< 16bit test */
    {
        uint16_t *p_uint16_src = (uint16_t *)src;
        uint16_t *p_uint16_dst = (uint16_t *)dst;

        test_count = size/sizeof(uint16_t);
        for(i = 0; i < test_count; i++)
        {
            if( *p_uint16_src != *p_uint16_dst )
            {
                BK_LOGD(NULL, "16bit test fail @ 0x%08X\r\nsystem halt!!!!!",(uint32_t)p_uint16_src);
                return -1;
            }
            p_uint16_src++;
            p_uint16_dst++;
        }

        BK_LOGD(NULL, "16bit test pass!!\r\n");
    }

    /**< 8bit test */
    {
        uint8_t *p_uint8_src = (uint8_t *)src;
        uint8_t *p_uint8_dst = (uint8_t *)dst;

        test_count = size/sizeof(uint8_t);
        for(i = 0; i < test_count; i++)
        {
            if( *p_uint8_src != *p_uint8_dst )
            {
                BK_LOGD(NULL, "8bit test fail @ 0x%08X\r\n", (uint32_t)p_uint8_src);
                return -1;
            }
            p_uint8_src++;
            p_uint8_dst++;
        }

        BK_LOGD(NULL, "8bit test pass!!\r\n");
    }

    /**< 32bit test write one address*/
    {
        uint32_t *p_uint32_src = (uint32_t *)src;
        uint32_t *p_uint32_dst = (uint32_t *)dst;
        uint32_t *p_uint32_next = (uint32_t *)dst + 1;

        test_count = size/sizeof(uint32_t);
        for(i = 0; i < test_count; i++)
        {
            *p_uint32_dst = *p_uint32_src;
            if(*p_uint32_next != *(p_uint32_dst + 1)) {
                BK_LOGD(NULL, "32bit test write one address fail @ 0x%08X\r\n!",(uint32_t)p_uint32_next);
                BK_LOGD(NULL, "==== next o:%08X,next n:%08X\r\n!",*p_uint32_next, *(p_uint32_dst + 1));
                return -1;
            }
            if( *p_uint32_src != *p_uint32_dst)
            {
                BK_LOGD(NULL, "32bit test write one address fail @ 0x%08X\r\n!",(uint32_t)p_uint32_src);
                BK_LOGD(NULL, "==== src:%08X,dest:%08X\r\n!",*p_uint32_src, *p_uint32_dst);
                return -1;
            }
            p_uint32_src++;
        }

        BK_LOGD(NULL, "32bit test write one address pass!!\r\n");
    }
#endif
    return 0;
}


static void cli_mem_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t address, size;

    if (argc >= 3) {
        address = strtoll(argv[1], NULL, 16);
        size = strtoll(argv[2], NULL, 16);
        BK_LOGD(NULL, "memtest,address: 0x%08X size: 0x%08X\r\n", address, size);

        mem_test(address, size, 0);
    } else if (argc == 1) {
        // auto_mem_test();
    } else {
        BK_LOGD(NULL, "memtest <addr> <length> \r\n");
    }
}


static void cli_mem_time(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t address, count, mode;
    if (argc >= 4) {
        address = strtoll(argv[1], NULL, 16);
        count = strtoll(argv[2], NULL, 16);
        mode = strtoll(argv[3], NULL, 16);
    } else {
        BK_LOGD(NULL, "memtime <addr> <count> <0:write,1:read> \r\n");
        return;
    }
    BK_LOGD(NULL, "memtime, address: 0x%08X count: 0x%08X, read=%d\r\n", address, count, mode);
    mem_time((uint32_t *)address, count, mode);
}


static void cli_memread_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t src, dest, size;

    if (argc >= 4) {
        src = strtoll(argv[1], NULL, 16);
        dest = strtoll(argv[2], NULL, 16);
        size = strtoll(argv[3], NULL, 16);
        BK_LOGD(NULL, "memread, src: 0x%08X dest: 0x%08X size: 0x%08X\r\n", src, dest, size);
        mem_read_test(src, dest, size);
    } else {
        BK_LOGD(NULL, "memread <src> <dest> <size> \r\n");
    }
}

#if CONFIG_MPU
void mpu_cfg(int index, uint32_t rbar, uint32_t rlar);
void mpu_init(void);

static void cli_mpucfg_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t index = 0, rbar = 0, rlar = 0;

    if (argc >= 4) {
        index = strtoll(argv[1], NULL, 10);
        rbar = strtoll(argv[2], NULL, 16);
        rlar = strtoll(argv[3], NULL, 16);
    } else {
        BK_LOGD(NULL, "mpucfg <index> <rbar> <rlar>\r\n");
        return;
    }

    BK_LOGD(NULL, "mpucfg, index:%d rbar: 0x%08X rlar: 0x%08X.\r\n", index, rbar, rlar);
    mpu_cfg(index, rbar, rlar);
}

void mpu_clear(uint32_t rnr);
static void cli_mpuclr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv) {
    uint32_t rnr;

    if (argc >= 2) {
        rnr = strtoll(argv[1], NULL, 10);
    } else {
        BK_LOGD(NULL, "mpuclr <rnr>.\r\n");
        return;
    }

    BK_LOGD(NULL, "mpuclr, rnr:%d.\r\n", rnr);
    mpu_clear(rnr);
}

void mpu_dump(void);
static void cli_mpudump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv) {
    mpu_dump();
}
#endif //#if CONFIG_MPU

#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
static void dump_hex(const uint8_t *ptr, size_t buflen)
{
    unsigned char *buf = (unsigned char*)ptr;
    int i, j;
    char line[80];

    for (i=0; i<buflen; i+=16)
    {
        int pos = 0;
        pos += snprintf(line + pos, sizeof(line) - pos, "%08X: ", i);

        for (j=0; j<16; j++)
        {
            if (i+j < buflen)
                pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", buf[i+j]);
            else
                pos += snprintf(line + pos, sizeof(line) - pos, "   ");
        }
        pos += snprintf(line + pos, sizeof(line) - pos, " ");

        for (j=0; j<16; j++)
        {
            if (i+j < buflen)
                pos += snprintf(line + pos, sizeof(line) - pos, "%c", __is_print(buf[i+j]) ? buf[i+j] : '.');
        }
        pos += snprintf(line + pos, sizeof(line) - pos, "\r\n");

        BK_DUMP_OUT("%s", line);
    }
}


static void cli_memory_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t address, size;

    if (argc >= 3) {
        address = strtoll(argv[1], NULL, 16);
        size = strtoll(argv[2], NULL, 16);
        BK_DUMP_OUT("dump,address: 0x%08X size: 0x%08X\r\n", address, size);

        if (argc == 3) {
            dump_hex((const uint8_t *)address, size);
        } else {
            bk_mem_dump("cli", address, size);
        }
    } else {
        BK_LOGD(NULL, "Usage: memdump <addr> <length>.\r\n");
        return;
    }
}

// src 
extern unsigned char _stext;
extern unsigned char __etext;
#define FLASH_CODE_REGION_START (uint32_t)&_stext
#define FLASH_CODE_REGION_END (uint32_t)&__etext

// Simple random number generator (linear congruential generator)
static uint32_t g_seed = 1;
static uint32_t simple_rand(void)
{
    g_seed = g_seed * 1103515245 + 12345;
    return g_seed;
}

static uint32_t simple_rand_range(uint32_t min, uint32_t max)
{
    if (min >= max) {
        return min;
    }
    return min + (simple_rand() % (max - min + 1));
}

// Copy data from flash code region to SRAM6
static void flash_to_sram_copy_test(uint32_t dst_base, uint32_t dst_size)
{
    uint32_t src_start = FLASH_CODE_REGION_START;
    uint32_t src_end = FLASH_CODE_REGION_END;
    uint32_t src_size = src_end - src_start;
    uint32_t dst_offset = 0;
    uint32_t min_copy_size = 1 * 1024;      // 1K
    uint32_t max_copy_size = 16 * 1024;     // 16K
    
    bk_printf("Flash to SRAM copy test started\r\n");
    bk_printf("Source: 0x%08X - 0x%08X (%u bytes)\r\n", src_start, src_end, src_size);
    bk_printf("Destination: 0x%08X (%u bytes)\r\n", dst_base, dst_size);
    
    uint32_t test_count = 0;
    
    while (1) {
        // Calculate remaining space in destination
        uint32_t remaining_dst = dst_size - dst_offset;
        
        if (remaining_dst < min_copy_size) {
            // Reset destination offset when full
            dst_offset = 0;
        }
        
        // Determine copy size (1K~16K, 1024-aligned)
        uint32_t copy_size;
        if (remaining_dst >= max_copy_size) {
            // Random size between 1K and 16K, aligned to 1024
            uint32_t random_blocks = simple_rand_range(1, 16); // 1~16 blocks
            copy_size = random_blocks * 1024;
        } else {
            // Use remaining space, aligned down to 1024
            copy_size = (remaining_dst / 1024) * 1024;
            if (copy_size < min_copy_size) {
                // Not enough space for even 1K, reset
                dst_offset = 0;
                continue;  // Skip to next iteration to recalculate copy_size
            }
        }
        
        // Random source address, ensuring enough space for copy_size
        // Source address must be aligned to 1024 and ensure copy_size fits
        uint32_t max_src_offset = (src_size >= copy_size) ? (src_size - copy_size) : 0;
        uint32_t max_src_blocks = max_src_offset / 1024;
        
        uint32_t src_offset;
        if (max_src_blocks > 0) {
            uint32_t random_blocks = simple_rand_range(0, max_src_blocks);
            src_offset = random_blocks * 1024;
        } else {
            src_offset = 0;
        }
        
        uint32_t src_addr = src_start + src_offset;
        
        // Double check: ensure source address + copy_size doesn't exceed src_end
        if (src_addr + copy_size > src_end) {
            src_addr = src_end - copy_size;
        }
        
        uint32_t dst_addr = dst_base + dst_offset;
        
        // Copy data
        memcpy((void *)dst_addr, (const void *)src_addr, copy_size);
        
        // Verify copied data
        int compare_result = memcmp((const void *)src_addr, (const void *)dst_addr, copy_size);
        
        test_count++;
        
        if (compare_result != 0) {
            // Data mismatch detected
            bk_printf("ERROR: Data mismatch at iteration %u!\r\n", test_count);
            bk_printf("  Source: 0x%08X, Destination: 0x%08X, Size: %u bytes\r\n", 
                      src_addr, dst_addr, copy_size);
            
            // Find first mismatch byte
            uint8_t *src_ptr = (uint8_t *)src_addr;
            uint8_t *dst_ptr = (uint8_t *)dst_addr;
            for (uint32_t i = 0; i < copy_size; i++) {
                if (src_ptr[i] != dst_ptr[i]) {
                    bk_printf("  First mismatch: src_addr=0x%08X(0x%02X), dst_addr=0x%08X(0x%02X)\r\n",
                              (uintptr_t)(src_ptr + i), src_ptr[i], (uintptr_t)(dst_ptr + i), dst_ptr[i]);
                    break;
                }
            }
        } else {
            // Data matches
            if (test_count % 100 == 0) {
                bk_printf("Test iteration %u: copied %u bytes from 0x%08X to 0x%08X, verified OK\r\n", 
                          test_count, copy_size, src_addr, dst_addr);
            }
        }
        
        // Update destination offset
        dst_offset += copy_size;
        
        // Small delay to prevent overwhelming the system
        rtos_delay_milliseconds(5);
    }
}

// Test task for flash to SRAM copy
static uint32_t dst_base;
static uint32_t dst_size;

static void flash_to_sram_test_task(void *arg)
{
    flash_to_sram_copy_test(dst_base, dst_size);
}

static void cli_flash_read_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    beken_thread_t test_thread;
    if (argc < 3) {
        BK_LOGI(NULL, "Usage: f2s_test <dst_base> <dst_size>\r\n");
        return;
    }

    dst_base = (uint32_t)os_strtoul(argv[1], NULL, 16);
    dst_size = (uint32_t)os_strtoul(argv[2], NULL, 16);
	BK_LOGI(NULL, "dst_base: 0x%08X, dst_size: 0x%08X\r\n", dst_base, dst_size);
    rtos_create_thread(&test_thread,
                        BEKEN_DEFAULT_WORKER_PRIORITY,
                        "flash_to_sram_test",
                        (beken_thread_function_t)flash_to_sram_test_task,
                        4096,
                        NULL);
}

#define MEM_CMD_CNT (sizeof(s_mem_commands) / sizeof(struct cli_command))
static const struct cli_command s_mem_commands[] = {
    {"memstack", "show stack memory usage", cli_memory_stack_cmd},
    {"memshow", "show free heap", cli_memory_free_cmd},
    {"f2s_test", "flash to sram [dst_base] [dst_size]", cli_flash_read_test_cmd},
#if CONFIG_MEM_DEBUG && CONFIG_FREERTOS
    {"memleak", "[show memleak", cli_memory_leak_cmd},
#endif
#if CONFIG_DEBUG_VERSION
    {"memdump", "<addr> <length>", cli_memory_dump_cmd},
    {"memset", "<addr> <value 1> [<value 2> ... <value n>]", cli_memory_set_cmd},
    {"memset_pattern", "<addr> <size> [pattern]", cli_memset_pattern_cmd},
    {"memset_addr", "<addr> <size>", cli_memset_addr_cmd},
    {"memcheck_pattern", "<addr> <size> [pattern]", cli_memcheck_pattern_cmd},
    {"memcheck_addr", "<addr> <size>", cli_memcheck_addr_cmd},
    {"memtest", "<addr> <length>", cli_mem_test},
    {"memtest_r", "<src> <dest> <size>", cli_memread_test},
    {"memtest_wr", "<addr> <count>", cli_memtest_wr_cmd},
	{"memtime", "<addr> <count> <0:write,1:read>", cli_mem_time},
#if CONFIG_MPU
    {"mpucfg", "<rnr> <rbar> <rlar>", cli_mpucfg_cmd},
    {"mpuclr", "<rnr>", cli_mpuclr_cmd},
    {"mpudump", "dump mpu config", cli_mpudump_cmd},
#endif //#if CONFIG_MPU
#endif //#if CONFIG_DEBUG_VERSION
#if CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS
    {"psram_malloc", "psram_malloc <length>", cli_psram_malloc_cmd},
    {"psram_free", "psram_free <addr>", cli_psram_free_cmd},
    {"psram_state", "psram_state", cli_psram_state_cmd},
#endif
};

int cli_mem_init(void)
{
	return cli_register_commands(s_mem_commands, MEM_CMD_CNT);
}
