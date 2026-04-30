#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/bk_frame_buffer.h>

#include "cli.h"
#include "media_service.h"
#include "h264d_gpu_display_demo.h"

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

static void bk_auxldo_enable(void)
{
	uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
	reg |= (0xFU << 28) | (0x2U << 23) | (0x7U << 19) | (0x7U << 15);
	reg &= ~(0xFU << 11);
	reg |= (0x8U << 11);
	REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

int main(void)
{
	bk_init();
	media_service_init();

	bk_printf("%s, %d, m55 running...\r\n", __func__, __LINE__);
	bk_auxldo_enable();

#ifdef CONFIG_FRAME_BUFFER
	bk_frame_buffer_init();
#endif

	cli_h264d_gpu_display_init();
	return 0;
}
