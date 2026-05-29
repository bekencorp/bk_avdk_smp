list(APPEND incs rtc)

if (CONFIG_AON_RTC)
	list(APPEND srcs rtc/aon_rtc_driver_64bit.c)
elseif (CONFIG_ANA_RTC)
	list(APPEND srcs rtc/ana_rtc_driver.c)
endif()

if (CONFIG_CLI AND CONFIG_AON_RTC_TEST)
	list(APPEND srcs rtc/aon_rtc_test.c)
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)
