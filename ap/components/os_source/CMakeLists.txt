if(CONFIG_FREERTOS)
	set(freertos_path)
	set(arch_name)

	set(incs)
	set(srcs)

	if(CONFIG_ARM_CORE_STAR)
		set(arch_name ARM_CM33_NTZ)
	elseif(CONFIG_ARM_CORE_CM52 OR CONFIG_ARM_CORE_CM55)
		set(arch_name ARM_CM55_NTZ)
	endif()

	if(CONFIG_FREERTOS_V10)
		set(freertos_path freertos_v10)
	elseif(CONFIG_FREERTOS_SMP)
		set(freertos_path freertos_smp_v2p0/FreeRTOS-Kernel)
	endif()

	# ===== Configure include paths ======
	if(CONFIG_FREERTOS_V10)
		set(incs ${freertos_path}/include)
	elseif(CONFIG_FREERTOS_SMP)
		set(incs
			${freertos_path}/include/freertos
			${freertos_path}/include/additions
		)
	endif()
	list(APPEND incs
		${freertos_path}/portable/GCC/${arch_name}/non_secure
	)

	# ===== Configure source files ======
	set(srcs
		${freertos_path}/croutine.c
		${freertos_path}/event_groups.c
		${freertos_path}/list.c
		${freertos_path}/queue.c
		${freertos_path}/tasks.c
		${freertos_path}/stream_buffer.c
		${freertos_path}/timers.c
		${freertos_path}/portable/GCC/${arch_name}/non_secure/port.c
		${freertos_path}/portable/GCC/${arch_name}/non_secure/portasm.c
		# ${freertos_path}/portable/Common/mpu_wrappers.c
		)
	# if(NOT (CONFIG_FULLY_HOSTED OR CONFIG_MEM_MGMT))
	# 	list(APPEND srcs ${freertos_path}/portable/MemMang/heap_4.c)
	# endif()

endif()


armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES bk_common bk_rtos core cmsis driver)

