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
    uint32_t dfsr;
    uint32_t afsr;
    uint32_t shcsr;
    uint32_t icsr;
    uint32_t vtor;
    uint32_t msplim;
    uint32_t psplim;
} bk_coredump_regs_t;

volatile cp_secure_fault_context_t
    g_cp_secure_fault_context[CP_SEC_DUMP_CONTEXT_COUNT]
    __attribute__((aligned(32)));
const uintptr_t g_cp_secure_fault_context_address =
    (uintptr_t)g_cp_secure_fault_context;

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

    /* Use the values captured at exception entry (before interrupts were
     * disabled); reading them here would always show PRIMASK=1 etc. */
    regs->primask = self->primask;
    regs->basepri = self->basepri;
    regs->faultmask = self->faultmask;

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
    regs->control = self->control;
    regs->mmfar = SCB->MMFAR;
    regs->bfar = SCB->BFAR;
    regs->cfsr = SCB->CFSR;
    regs->hfsr = SCB->HFSR;
    regs->dfsr = SCB->DFSR;     // debug fault status
    regs->afsr = SCB->AFSR;     // auxiliary (vendor) fault status
    regs->shcsr = SCB->SHCSR;   // system handler control/state
    regs->icsr = SCB->ICSR;     // active/pending exception numbers
    regs->vtor = SCB->VTOR;     // vector table base
    regs->msplim = __get_MSPLIM();
    regs->psplim = __get_PSPLIM();

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
    bk_coredump_write_registers("44 DFSR", regs->dfsr);
    bk_coredump_write_registers("45 AFSR", regs->afsr);
    bk_coredump_write_registers("46 SHCSR", regs->shcsr);
    bk_coredump_write_registers("47 ICSR", regs->icsr);
    bk_coredump_write_registers("48 VTOR", regs->vtor);
    bk_coredump_write_registers("49 MSPLIM", regs->msplim);
    bk_coredump_write_registers("50 PSPLIM", regs->psplim);
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

static void coredump_check_fault_addr_valid(bk_coredump_regs_t *regs)
{
    /* MMFAR/BFAR only hold a meaningful address when the corresponding VALID
     * bit in CFSR is set; otherwise their value is stale and must be ignored. */
    if ((regs->cfsr & SCB_CFSR_MMARVALID_Msk) == 0) {
        bk_coredump_write_prompt("Note: MMFAR invalid (CFSR.MMARVALID=0), ignore its value.\r\n");
    }
    if ((regs->cfsr & SCB_CFSR_BFARVALID_Msk) == 0) {
        bk_coredump_write_prompt("Note: BFAR invalid (CFSR.BFARVALID=0), ignore its value.\r\n");
    }
}

static void bk_coredump_registers_arm(bk_exception_t *self)
{
    bk_coredump_regs_t regs;
    coredump_save_registers(self, &regs);
    coredump_write_arm_registers(&regs);
    coredump_check_fault_addr_valid(&regs);
    coredump_check_stack_overflow(&regs);
    coredump_traceback(&regs);
}

void bk_coredump_secure_registers(const cp_secure_fault_context_t *context)
{
    bk_coredump_regs_t regs = {0};
    bool source_secure = (context->flags & CP_SEC_DUMP_FLAG_SOURCE_SECURE) != 0U;
    bool source_psp = (context->flags & CP_SEC_DUMP_FLAG_SOURCE_PSP) != 0U;

    regs.r0 = context->r[0];
    regs.r1 = context->r[1];
    regs.r2 = context->r[2];
    regs.r3 = context->r[3];
    regs.r4 = context->r[4];
    regs.r5 = context->r[5];
    regs.r6 = context->r[6];
    regs.r7 = context->r[7];
    regs.r8 = context->r[8];
    regs.r9 = context->r[9];
    regs.r10 = context->r[10];
    regs.r11 = context->r[11];
    regs.r12 = context->r[12];
    regs.sp = context->sp;
    regs.lr = context->lr;
    regs.pc = context->pc;
    regs.xpsr = context->xpsr;
    regs.msp = source_secure ? context->msp_s : context->msp_ns;
    regs.psp = source_secure ? context->psp_s : context->psp_ns;
    if (source_psp) {
        regs.psp = regs.sp;
    } else {
        regs.msp = regs.sp;
    }
    regs.primask = source_secure ? context->primask_s : context->primask_ns;
    regs.basepri = source_secure ? context->basepri_s : context->basepri_ns;
    regs.faultmask = source_secure ? context->faultmask_s : context->faultmask_ns;
    regs.control = source_secure ? context->control_s : context->control_ns;
    regs.fpscr = context->fpscr_s;
    regs.exception_lr = context->exception_lr;
    regs.mmfar = context->mmfar_s;
    regs.bfar = context->bfar_s;
    regs.cfsr = context->cfsr_s;
    regs.hfsr = context->hfsr_s;

    coredump_write_arm_registers(&regs);
    bk_coredump_write_registers("51 IPSR", context->ipsr);
    bk_coredump_write_registers("52 FLAGS", context->flags);
    bk_coredump_write_registers("53 FSP", context->frame_sp);
    bk_coredump_write_registers("54 MSP_S", context->msp_s);
    bk_coredump_write_registers("55 PSP_S", context->psp_s);
    bk_coredump_write_registers("56 MSP_NS", context->msp_ns);
    bk_coredump_write_registers("57 PSP_NS", context->psp_ns);
    bk_coredump_write_registers("58 CTRL_S", context->control_s);
    bk_coredump_write_registers("59 CTRL_NS", context->control_ns);
    bk_coredump_write_registers("60 SFSR", context->sfsr);
    bk_coredump_write_registers("61 SFAR", context->sfar);

    if (!source_secure && context->frame_valid != 0U) {
        coredump_traceback(&regs);
    } else {
        bk_coredump_write_prompt(
            "Secure-source stack traceback skipped; registers were copied before BLXNS.\r\n");
    }
}

void bk_coredump_secure_fault_callback(void *context_arg)
{
    cp_secure_fault_context_t *context =
        (cp_secure_fault_context_t *)context_arg;
    bool valid_context = false;

    for (uint32_t i = 0; i < CP_SEC_DUMP_CONTEXT_COUNT; i++) {
        if (context == (cp_secure_fault_context_t *)&g_cp_secure_fault_context[i]) {
            valid_context = true;
            break;
        }
    }

    if (valid_context &&
        context->magic == CP_SEC_DUMP_CONTEXT_MAGIC &&
        context->version == CP_SEC_DUMP_ABI_VERSION &&
        context->size == sizeof(*context) &&
        context->frame_valid != 0U) {
        bk_exception_handler_from_secure(context);
        while (1) {
        }
    }

    bk_reboot_ex(RESET_SOURCE_SECURE_FAULT);
    while (1) {
    }
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
