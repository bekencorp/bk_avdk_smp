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

if (CONFIG_SECURITY_OTA)
list(APPEND srcs ota_security/security_ota.c)

	if (CONFIG_OTA_HTTPS)
		list(APPEND srcs ota_security/security_ota_https.c)
	endif()

	if (CONFIG_OTA_HTTP)
		list(APPEND srcs ota_security/security_ota_http.c)
	endif()
else()
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
endif()

if (CONFIG_CLI AND (CONFIG_OTA_HTTP OR CONFIG_OTA_HTTPS OR CONFIG_HTTP_AB_PARTITION OR CONFIG_SECURITY_OTA))
	list(APPEND srcs ota_test.c)
endif()
endif()
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common bk_netif bk_ps fatfs bk_vfs bk_init bk_wifi compal http bk_https psa_mbedtls bk_cli
	)
