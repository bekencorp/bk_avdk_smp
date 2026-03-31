// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include "bk_arch.h"
#include "memory.h"
/* ========================================================================== */

static bool disassembly_ins_is_bl_blx(uint32_t addr) {
    uint16_t ins1 = *((uint16_t *)addr);
    uint16_t ins2 = *((uint16_t *)(addr + 2));

#define BL_INS_MASK         0xF800
#define BL_INS_HIGH         0xF800
#define BL_INS_LOW          0xF000
#define BLX_INX_MASK        0xFF00
#define BLX_INX             0x4700

    if ((ins2 & BL_INS_MASK) == BL_INS_HIGH && (ins1 & BL_INS_MASK) == BL_INS_LOW) {
        return true;
    } else if ((ins2 & BLX_INX_MASK) == BLX_INX) {
        return true;
    } else {
        return false;
    }
}

bool check_lr_valid(uint32_t lr)
{
    if (!bk_check_addr_in_code_section(lr)) {
        return false;
    }
    return disassembly_ins_is_bl_blx(lr);
}

uint32_t *bk_find_next_valid_lr_pos(uint32_t *start, uint32_t *end)
{
    for (uint32_t *i = start; i < end; i++) {
        if (!bk_check_addr_in_ram((uint32_t)i)) {
            break;
        }
        uint32_t addr = *i;
        if ((addr & 0x1) == 0) {
            continue;
        }
        addr &= ~1;
        addr -= sizeof(size_t);
        if (!check_lr_valid(addr)) {
            continue;
        }
        return i;
    }
    return NULL;
}

static void call_traceback(uint32_t stack_top, uint32_t stack_bottom, uint32_t lr)
{
    uint32_t last_lr = 0;
    BK_RAW_LOGW(NULL, "Traceback:\r\n");
    BK_RAW_LOGW(NULL, "arm-none-eabi-addr2line -piaf -e app.elf ");
    for (uint32_t i = stack_top; i < stack_bottom; i += 4) {
        uint32_t addr = *(uint32_t *)i;
        if (addr == lr) {
            stack_top = i;
            break;
        }
    }
    for (uint32_t i = stack_top; i < stack_bottom; i += 4) {
        uint32_t addr = *(uint32_t *)i;
        if ((addr & 0x1) == 0) {
            continue;
        }
        addr &= ~1;
        addr -= sizeof(size_t);
        if (!check_lr_valid(addr)) {
            continue;
        }
        if (addr == last_lr) {
            continue;
        }
        BK_RAW_LOGW(NULL, "%p ", (void *)addr);
        last_lr = addr;
    }
    BK_RAW_LOGW(NULL, "\r\n");
}

__attribute__((noinline)) void bk_traceback(void)
{
    uint32_t lr = __get_LR();
    uint32_t sp = __get_SP();
    uint32_t msp = __get_MSP();
    uint32_t psp = __get_PSP();
    uint32_t stack_bottom = 0;
    BK_RAW_LOGV(NULL, "Traceback:\r\n");
    if (sp == msp) {
        // main stack
        BK_RAW_LOGV(NULL, "Main stack\r\n");
        stack_bottom = bk_get_msp_bottom();
    } else if (sp == psp) {
        // process stack
        BK_RAW_LOGV(NULL, "Process stack\r\n");
        stack_bottom = bk_get_current_stack_bottom();
    } else {
        // unknown stack
        BK_RAW_LOGW(NULL, "Unknown stack\r\n");
        BK_RAW_LOGW(NULL, "lr: %p, sp: %p, msp: %p, psp: %p\r\n", lr, sp, msp, psp);
    }
    if (stack_bottom == 0) {
        BK_RAW_LOGW(NULL, "Failed to get stack bottom\r\n");
        return;
    }
    call_traceback(sp, stack_bottom, lr);
}
