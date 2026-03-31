// Copyright 2020-2024 Beken
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

#include <driver/io_matrix.h>
#include <driver/hal/hal_gpio_types.h>
#include "sdio_storage_driver.h"
#include "sys_a35_ll.h"
#include "sys_ana_ll.h"
#include <driver/gicv2.h>
#include <driver/int_types.h>
#include <driver/int.h>

#include "mmc_dev.h"
#include "sdhci.h"

bk_err_t bk_sdio_storage_driver_init(void)
{
        /*TODO:wangzhilei*/
        bk_int_isr_register(INT_SRC_SDIO0, (int_group_isr_t)sdhci_irq_instance0, NULL);

        bk_int_isr_register(INT_SRC_SDIO1, (int_group_isr_t)sdhci_irq_instance1, NULL);

#if CONFIG_SDIO_DWC_TEST
        int bk_sdio_host_register_cli_test_feature(void);
        bk_sdio_host_register_cli_test_feature();
#endif

        return BK_OK;
}

bk_err_t bk_sdio_host_driver_deinit(void)
{
        /*TODO:wangzhilei*/
        gicv2_disable_interrupt(INT_SRC_SDIO0);
        bk_int_isr_unregister(INT_SRC_SDIO0);

        gicv2_disable_interrupt(INT_SRC_SDIO1);
        bk_int_isr_unregister(INT_SRC_SDIO1);

        return BK_OK;
}

// eof

