#include "bk_arch.h"
#include "bk_coredump.h"
#include "os/os.h"  // for rtos_get_core_id
#include "memory.h"

#if CONFIG_ARCH_CORTEX_M
void bk_coredump_registers(bk_exception_t *self) __attribute__((alias("bk_coredump_registers_arm")));
const char *bk_coredump_get_fault_type(void) __attribute__((alias("bk_coredump_get_fault_type_arm")));
#endif

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t msp;
    uint32_t psp;
    uint32_t primask;
    uint32_t basepri;
    uint32_t faultmask;
    uint32_t control;
    uint32_t fpscr;
    uint32_t exception_lr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t cfsr;
    uint32_t hfsr;
} bk_coredump_regs_t;

static inline bool is_dump_from_thread(uint32_t lr)
{
    return lr & (1UL << 2);
}

static inline bool is_fpu_enabled(uint32_t lr)
{
    return (lr & (1UL << 4)) == 0;
}

static inline bool is_need_padding_word(uint32_t xpsr)
{
    return xpsr & (1UL << 9);
}

// NOTE: The register restore for task_watchdog is not fully accurate.
// r4~r11 register data will be lost in this scenario.
static inline bool is_from_task_wdt(bool from_thread, uint32_t reset_reason, uint32_t xpsr)
{
    int task_wdt_int_id = BK_DUMP_TASK_WD_TIMER_INTERRUPT + 16;
    return !from_thread && reset_reason == RESET_SOURCE_CRASH_ASSERT &&
            ((xpsr & 0x1FF) == task_wdt_int_id);
}

static void coredump_save_registers(bk_exception_t *self, bk_coredump_regs_t *regs)
{
    uint32_t *msp = (uint32_t *)(self->sp);
    uint32_t lr = self->lr;
    regs->r4 = msp[-8];
    regs->r5 = msp[-7];
    regs->r6 = msp[-6];
    regs->r7 = msp[-5];
    regs->r8 = msp[-4];
    regs->r9 = msp[-3];
    regs->r10 = msp[-2];
    regs->r11 = msp[-1];

    regs->msp = __get_MSP();
    regs->psp = __get_PSP();

    regs->primask = __get_PRIMASK();
    regs->basepri = __get_BASEPRI();
    regs->faultmask = __get_FAULTMASK();

    regs->fpscr = __get_FPSCR();

    bool from_thread = is_dump_from_thread(lr);
    uint32_t *except_stack = (uint32_t *)(from_thread ? regs->psp : (uint32_t)msp);
    if (is_from_task_wdt(from_thread, self->reset_reason, except_stack[7])) { // from wdt interrupt
        except_stack = (uint32_t *)regs->psp;
        from_thread = true;
    }
    uint32_t stack_adj = 8 * sizeof(uint32_t);

    if(is_fpu_enabled(lr)) {  // fpu is enabled
        stack_adj += 18 * sizeof(uint32_t);  // 18 FPU registers
    }

    regs->r0 = except_stack[0];
    regs->r1 = except_stack[1];
    regs->r2 = except_stack[2];
    regs->r3 = except_stack[3];
    regs->r12 = except_stack[4];
    regs->lr = except_stack[5];
    regs->pc = except_stack[6];
    regs->xpsr = except_stack[7];
    regs->exception_lr = lr;
    regs->control = __get_CONTROL();
    regs->mmfar = SCB->MMFAR;
    regs->bfar = SCB->BFAR;
    regs->cfsr = SCB->CFSR;
    regs->hfsr = SCB->HFSR;

    if(is_need_padding_word(regs->xpsr)) { //  padding word flag
        stack_adj += 1 * sizeof(uint32_t);
    }
    regs->sp = (uint32_t)except_stack + stack_adj;
    if (from_thread) {
        regs->psp = regs->sp;
    } else {
        regs->msp = regs->sp;
    }
}


static void coredump_write_arm_registers(bk_coredump_regs_t *regs)
{
    bk_coredump_write_meta_info(COREDUMP_REGISTERS_INFO, (void *)rtos_get_core_id());
    bk_coredump_write_registers("0 r0", regs->r0);
    bk_coredump_write_registers("1 r1", regs->r1);
    bk_coredump_write_registers("2 r2", regs->r2);
    bk_coredump_write_registers("3 r3", regs->r3);
    bk_coredump_write_registers("4 r4", regs->r4);
    bk_coredump_write_registers("5 r5", regs->r5);
    bk_coredump_write_registers("6 r6", regs->r6);
    bk_coredump_write_registers("7 r7", regs->r7);
    bk_coredump_write_registers("8 r8", regs->r8);
    bk_coredump_write_registers("9 r9", regs->r9);
    bk_coredump_write_registers("10 r10", regs->r10);
    bk_coredump_write_registers("11 r11", regs->r11);
    bk_coredump_write_registers("12 r12", regs->r12);
    bk_coredump_write_registers("14 sp", regs->sp);
    bk_coredump_write_registers("15 lr", regs->lr);
    bk_coredump_write_registers("16 pc", regs->pc);
    bk_coredump_write_registers("17 xpsr", regs->xpsr);
    bk_coredump_write_registers("18 msp", regs->msp);
    bk_coredump_write_registers("19 psp", regs->psp);
    bk_coredump_write_registers("20 primask", regs->primask);
    bk_coredump_write_registers("21 basepri", regs->basepri);
    bk_coredump_write_registers("22 faultmask", regs->faultmask);
    bk_coredump_write_registers("23 fpscr", regs->fpscr);
    bk_coredump_write_registers("31 ER", regs->exception_lr);
    bk_coredump_write_registers("32 control", regs->control);
    bk_coredump_write_registers("40 MMFAR", regs->mmfar);
    bk_coredump_write_registers("41 BFAR", regs->bfar);
    bk_coredump_write_registers("42 CFSR", regs->cfsr);
    bk_coredump_write_registers("43 HFSR", regs->hfsr);
}

static void coredump_traceback(bk_coredump_regs_t *regs)
{
    uint32_t *start;
    uint32_t *end;
    if (regs->psp == regs->sp) {
        start = (uint32_t *)regs->psp;
        end = (uint32_t *)bk_get_current_stack_bottom();
    } else {
        start = (uint32_t *)regs->msp;
        end = (uint32_t *)bk_get_msp_bottom();
    }
    if (start >= end) {
        return;
    }

    uint32_t lr_trace[16] = {0};
    lr_trace[0] = regs->pc;
    lr_trace[1] = regs->lr;
    uint32_t count = 2;
    while (start < end && count < 16) {
        uint32_t *lr_pos = bk_find_next_valid_lr_pos(start, end);
        if (lr_pos != NULL) {
            lr_trace[count] = ((*lr_pos) & ~1) - sizeof(size_t);
            count++;
        } else {
            break;
        }
        start = lr_pos + 1;
    }
    bk_mem_addr_t lr_trace_addr = {
        .start_addr = (uint32_t)lr_trace,
        .size = count,
    };
    bk_coredump_write_meta_info(COREDUMP_TRACEBACK_INFO, (void *)&lr_trace_addr);
}

char * vTaskName(void);
static void coredump_check_stack_overflow(bk_coredump_regs_t *regs)
{
    if ((regs->cfsr & SCB_CFSR_STKOF_Msk) != 0) {
        bk_coredump_write_prompt("Stack overflow detected: The fault context may be inaccurate.\r\n");
        if (regs->psp == regs->sp) {
            bk_coredump_write_prompt("Current Task stack overflow: %s\r\n", vTaskName());
        } else {
            bk_coredump_write_prompt("MSP stack overflow\r\n");
        }
    }
}

static void bk_coredump_registers_arm(bk_exception_t *self)
{
    bk_coredump_regs_t regs;
    coredump_save_registers(self, &regs);
    coredump_write_arm_registers(&regs);
    coredump_check_stack_overflow(&regs);
    coredump_traceback(&regs);
}

static const char * const fault_type[] =
{
    [0]  = NULL,
    [1]  = NULL,
    [2]  = "Watchdog",
    [3]  = "HardFault",
    [4]  = "MemFault",
    [5]  = "BusFault",
    [6]  = "UsageFault",
    [7]  = "SecureFault",
    [8]  = NULL,
    [9]  = NULL,
    [10] = NULL,
    [11] = "SVC",
    [12] = "DebugFault",
    [13] = NULL,
    [14] = "PendSV",
    [15] = "SysTick",
};

static const char *bk_coredump_get_fault_type_arm(void)
{
    uint32_t mcause = __get_xPSR() & 0x1FF;
    if (mcause <= 0x0F) {
        return fault_type[mcause];
    }
    return "Unknown";
}
