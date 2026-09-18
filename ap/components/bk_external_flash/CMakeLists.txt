# bk_external_flash: off-chip QSPI NOR/NAND flash driver component.
#
# Holds the QSPI NOR/NAND protocol engines, bad-block management and the
# JEDEC-probed device-descriptor tables. Kept separate from bk_peripheral
# (board devices) and from the middleware QSPI controller. Compiled based on the
# QSPI flash type selected in the middleware QSPI Kconfig
# (CONFIG_QSPI_NOR_FLASH / CONFIG_QSPI_NAND_FLASH); public bk_qspi_flash_* API
# lives in ap/include.

set(srcs "")
set(incs "")
set(priv_incs "")
set(GLOBAL_FUNCTION_SYMBOLS "")

if (CONFIG_QSPI_NAND_FLASH)
	list(APPEND srcs
		qspi_flash/qspi_nand_flash.c
		qspi_flash/qspi_nand_bbm.c
		qspi_flash/nand/qspi_nand_zb35q01.c
		qspi_flash/nand/qspi_nand_gd5f05.c
	)
	list(APPEND priv_incs qspi_flash)
	# Device descriptors register into .qspi_nand_device_list via a static entry;
	# force-keep the descriptor globals for non-whole-archive links.
	list(APPEND GLOBAL_FUNCTION_SYMBOLS
		qspi_nand_zb35q01_dev
		qspi_nand_gd5f05m7ue_dev
		qspi_nand_gd5f05m7re_dev
	)
elseif (CONFIG_QSPI_NOR_FLASH)
	list(APPEND srcs
		qspi_flash/qspi_nor_flash.c
		qspi_flash/nor/qspi_nor_gd25.c
		qspi_flash/nor/qspi_nor_puya_p25q32.c
		qspi_flash/nor/qspi_nor_xmc_xm25qh256.c
	)
	list(APPEND priv_incs qspi_flash)
	list(APPEND GLOBAL_FUNCTION_SYMBOLS
		qspi_nor_gd25_dev
		qspi_nor_p25q32_dev
		qspi_nor_xm25qh256_dev
	)
endif()

set(priv_req bk_common)
if ((CONFIG_QSPI_NAND_FLASH OR CONFIG_QSPI_NOR_FLASH) AND CONFIG_CLI AND CONFIG_QSPI_TEST)
	# cli.h for the flash/NAND CLI test commands (only built with QSPI_TEST).
	list(APPEND srcs qspi_flash/qspi_flash_test.c)
	list(APPEND priv_req bk_cli)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_INCLUDE_DIRS "${priv_incs}" PRIV_REQUIRES "${priv_req}")

foreach(symbol ${GLOBAL_FUNCTION_SYMBOLS})
	target_link_libraries(${COMPONENT_LIB} INTERFACE "-u ${symbol}")
endforeach()
