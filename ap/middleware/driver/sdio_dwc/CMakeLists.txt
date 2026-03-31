if (CONFIG_SUPPORT_SDIO_DWC)
	if (CONFIG_SDIO_DWC_V1P0)
	list(APPEND incs sdio_dwc sdio_dwc/v1p0)
	list(APPEND srcs sdio_dwc/v1p0/sdio_storage_driver.c)

	if (CONFIG_SDIO_DWC_TEST)
		list(APPEND srcs sdio_dwc/v1p0/sdio_dwc_test.c)
	endif ()
	elseif (CONFIG_SDIO_DWC_V2P0)
	list(APPEND incs sdio_dwc 
                sdio_dwc/v2p0
                sdio_dwc/v2p0/config
                sdio_dwc/v2p0/inc
                sdio_dwc/v2p0/inc/mmc
                sdio_dwc/v2p0/core
                sdio_dwc/v2p0/platform
                sdio_dwc/v2p0/drv/clock
                sdio_dwc/v2p0/drv/core
                sdio_dwc/v2p0/drv/mmc/dwc-mshc
                sdio_dwc/v2p0/utils/terminal
                sdio_dwc/v2p0/utils/parser
                sdio_dwc/v2p0/test/core)

	list(APPEND srcs sdio_dwc/v2p0/drv/mmc/dwc-mshc/emmc_dev.c
                sdio_dwc/v2p0/drv/mmc/dwc-mshc/mmc_core.c
                sdio_dwc/v2p0/drv/mmc/dwc-mshc/mmc_dev.c
                sdio_dwc/v2p0/drv/mmc/dwc-mshc/sd_cmds.c
                sdio_dwc/v2p0/drv/mmc/dwc-mshc/sdhci.c
                sdio_dwc/v2p0/drv/mmc/dwc-mshc/sdhci_hal.c
                sdio_dwc/v2p0/drv/core/cee.c
                sdio_dwc/v2p0/drv/clock/clkcore.c
                sdio_dwc/v2p0/drv/clock/mmcm_clk.c
                sdio_dwc/v2p0/core/clink.c
                sdio_dwc/v2p0/core/clog.c
                sdio_dwc/v2p0/core/cmsg_queue.c
                sdio_dwc/v2p0/core/csem.c
                sdio_dwc/v2p0/core/ctask.c
                sdio_dwc/v2p0/core/cwatch.c
                sdio_dwc/v2p0/core/delay.c
                sdio_dwc/v2p0/core/fs.c
                sdio_dwc/v2p0/core/init.c
                sdio_dwc/v2p0/core/regdesc.c
                sdio_dwc/v2p0/os/freertos/ctask_freertos.c
                sdio_dwc/v2p0/os/freertos/fs_freertos.c
                sdio_dwc/v2p0/os/freertos/ctask_freertos_sem.c
                sdio_dwc/v2p0/utils/parser/parser.c
                sdio_dwc/v2p0/platform/hal.c
                sdio_dwc/v2p0/sdio_storage_driver.c
                sdio_dwc/v2p0/utils/terminal/stdcmd.c
                sdio_dwc/v2p0/utils/terminal/terminal.c)

	if (CONFIG_SDIO_DWC_TEST)
		list(APPEND srcs sdio_dwc/v2p0/sdio_dwc_test.c
                sdio_dwc/v2p0/drv/mmc/dwc-mshc/mmc_test.c
                sdio_dwc/v2p0/drv/mmc/dwc-mshc/sd_card.c
                sdio_dwc/v2p0/test/core/tee.c
                sdio_dwc/v2p0/main/mmc/dwc-mshc/mshc_main.c)
	endif ()
	elseif (CONFIG_SDIO_DWC_V2P1)
	list(APPEND incs sdio_dwc 
                sdio_dwc/v2p1
                sdio_dwc/v2p1/config
                sdio_dwc/v2p1/inc
                sdio_dwc/v2p1/inc/mmc
                sdio_dwc/v2p1/core
                sdio_dwc/v2p1/platform
                sdio_dwc/v2p1/drv/clock
                sdio_dwc/v2p1/drv/core
                sdio_dwc/v2p1/drv/mmc/dwc-mshc
                sdio_dwc/v2p1/utils/terminal
                sdio_dwc/v2p1/utils/parser
                sdio_dwc/v2p1/test/core)

	list(APPEND srcs sdio_dwc/v2p1/drv/mmc/dwc-mshc/mmc_dev.c
                sdio_dwc/v2p1/drv/mmc/dwc-mshc/mmc_core.c
                sdio_dwc/v2p1/drv/mmc/dwc-mshc/sd_cmds.c
                sdio_dwc/v2p1/drv/mmc/dwc-mshc/sdhci.c
                sdio_dwc/v2p1/drv/mmc/dwc-mshc/sdhci_hal.c
                sdio_dwc/v2p1/drv/core/cee.c
                sdio_dwc/v2p1/drv/clock/clkcore.c
                sdio_dwc/v2p1/drv/clock/mmcm_clk.c
                sdio_dwc/v2p1/drv/mmc/dwc-mshc/sd_card.c
                sdio_dwc/v2p1/core/clink.c
                sdio_dwc/v2p1/core/clog.c
                sdio_dwc/v2p1/core/cmsg_queue.c
                sdio_dwc/v2p1/core/csem.c
                sdio_dwc/v2p1/core/ctask.c
                sdio_dwc/v2p1/core/cwatch.c
                sdio_dwc/v2p1/core/delay.c
                sdio_dwc/v2p1/core/fs.c
                sdio_dwc/v2p1/core/init.c
                sdio_dwc/v2p1/core/regdesc.c
                sdio_dwc/v2p1/os/freertos/ctask_freertos.c
                sdio_dwc/v2p1/os/freertos/fs_freertos.c
                sdio_dwc/v2p1/os/freertos/ctask_freertos_sem.c
                sdio_dwc/v2p1/platform/hal.c
                sdio_dwc/v2p1/sdio_storage_driver.c)

	if (CONFIG_SDIO_DWC_TEST)
		list(APPEND srcs sdio_dwc/v2p1/sdio_dwc_test.c)
	endif ()
	endif ()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
