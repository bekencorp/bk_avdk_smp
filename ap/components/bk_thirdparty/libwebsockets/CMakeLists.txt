#
# libwebsockets - small server side websockets and web server implementation
#
# Copyright (C) 2010 - 2022 Andy Green <andy@warmcat.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#

cmake_minimum_required(VERSION 3.10...4.0)

# Armino (beken) component build: register with real srcs/incs, no project()
# When CONFIG_LIBWEBSOCKETS=n, register with empty srcs so the component is not compiled (like libsrtp2).
if(DEFINED COMPONENT_DIR)
	if(CONFIG_LIBWEBSOCKETS)
		include(${COMPONENT_DIR}/CMakeLists-armino.cmake)
		armino_component_register(
			SRCS ${srcs}
			INCLUDE_DIRS ${incs}
			PRIV_REQUIRES bk_common os_source lwip_intf_v2_1 bk_vfs bk_rtos driver psa_mbedtls
		)
		# os_source exports include/freertos, so use flat <FreeRTOS.h> not <freertos/FreeRTOS.h>
		# Enable X.509 cert create/write for lws_x509_create_self_signed (see configs/config_mbedtls.h)
		component_compile_definitions(LWS_AMAZON_RTOS MBEDTLS_USER_CONFIG_FILE="config_mbedtls.h")
	else()
		set(srcs)
		set(incs)
		armino_component_register(
			SRCS ${srcs}
			INCLUDE_DIRS ${incs}
			PRIV_REQUIRES bk_common os_source lwip_intf_v2_1 bk_vfs bk_rtos driver psa_mbedtls
		)
	endif()
else()
	message(FATAL_ERROR "libwebsockets: must be built as an Armino component (COMPONENT_DIR is not set).")
endif()
