set(incs include include/bk_private)
set(srcs)

if(CONFIG_OTA_FUNCTION)
if (CONFIG_OTA_HASH_FUNCTION)
	list(APPEND srcs ota_checksum/ota_hash.c)
endif()

# CheckSumUtils.c is also provided by ap/middleware/driver/flash/CheckSumUtils.c
# (a superset of this file). Reuse driver's version to avoid multiple-definition
# linker errors. Header in include/bk_private/CheckSumUtils.h is still used.
#list(APPEND srcs ota_checksum/CheckSumUtils.c)

# Non-secure OTA framework. The legacy TF-M FWU secure OTA (ota_security/) has
# been removed; secure OTA is now the DIRECT_XIP A/B back-end in ota_secure_xip/.
if (CONFIG_OTA_HTTPS)
	list(APPEND srcs ota_nosecurity/ota_https.c)
endif()

if (CONFIG_OTA_HTTP)
	list(APPEND srcs ota_nosecurity/ota_http.c)
endif()
list(APPEND srcs
	ota.c
	ota_common.c
	ota_base_drv.c)

# Secure XIP A/B OTA back-end, layered on the non-secure OTA framework.
if (CONFIG_SECURE_OTA_XIP)
	list(APPEND srcs
		ota_secure_xip/ota_boot_param.c
		ota_secure_xip/ota_secure_xip.c)
endif()

if (CONFIG_CLI AND (CONFIG_OTA_HTTP OR CONFIG_OTA_HTTPS OR CONFIG_HTTP_AB_PARTITION OR CONFIG_SECURE_OTA_XIP))
	list(APPEND srcs ota_test.c)
endif()
endif()
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common bk_netif bk_ps fatfs bk_vfs bk_init bk_wifi compal http bk_https psa_mbedtls bk_cli
	)
