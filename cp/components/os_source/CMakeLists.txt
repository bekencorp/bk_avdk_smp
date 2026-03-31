if(CONFIG_FREERTOS)
	if(CONFIG_FREERTOS_V10)
		set(incs freertos_v10/include)
		set(srcs
			freertos_v10/croutine.c
			freertos_v10/event_groups.c
			freertos_v10/list.c
			freertos_v10/queue.c
			freertos_v10/tasks.c
			freertos_v10/stream_buffer.c
			freertos_v10/timers.c
			freertos_v10/portable/MemMang/heap_4.c
			)

		if(CONFIG_ARM_CORE_STAR)
			list(APPEND incs
				freertos_v10/portable/GCC/ARM_CM33_NTZ/non_secure
				)

			list(APPEND srcs
				freertos_v10/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
				freertos_v10/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
				freertos_v10/portable/Common/mpu_wrappers.c
				)
		elseif(CONFIG_ARM_CORE_CM52 OR CONFIG_ARM_CORE_CM55)
			list(APPEND incs
				freertos_v10/portable/GCC/ARM_CM55_NTZ/non_secure
				)

			list(APPEND srcs
				freertos_v10/portable/GCC/ARM_CM55_NTZ/non_secure/port.c
				freertos_v10/portable/GCC/ARM_CM55_NTZ/non_secure/portasm.c
				freertos_v10/portable/Common/mpu_wrappers.c
				)
		else()
		endif()
		
		if(CONFIG_FULLY_HOSTED OR CONFIG_MEM_MGMT)
			list(REMOVE_ITEM srcs freertos_v10/portable/MemMang/heap_4.c)
		endif()
	elseif(CONFIG_FREERTOS_SMP)
			set(incs
				freertos_smp_v2p0/FreeRTOS-Kernel/include/freertos
				freertos_smp_v2p0/FreeRTOS-Kernel/include/additions
			)
			set(srcs
				freertos_smp_v2p0/FreeRTOS-Kernel/croutine.c
				freertos_smp_v2p0/FreeRTOS-Kernel/event_groups.c
				freertos_smp_v2p0/FreeRTOS-Kernel/list.c
				freertos_smp_v2p0/FreeRTOS-Kernel/queue.c
				freertos_smp_v2p0/FreeRTOS-Kernel/tasks.c
				freertos_smp_v2p0/FreeRTOS-Kernel/stream_buffer.c
				freertos_smp_v2p0/FreeRTOS-Kernel/timers.c
				freertos_smp_v2p0/FreeRTOS-Kernel/portable/MemMang/heap_4.c
				)

			if(CONFIG_ARM_CORE_STAR)
				list(APPEND incs
					freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure
					)

				list(APPEND srcs
					freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
					freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
					)
			endif()
			
			if(CONFIG_ARM_CORE_CM52)
				list(APPEND incs
					freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM52_NTZ/non_secure
					)

				list(APPEND srcs
					freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM52_NTZ/non_secure/port.c
					freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM52_NTZ/non_secure/portasm.c
					)
			endif()
			
			if(CONFIG_FULLY_HOSTED OR CONFIG_MEM_MGMT)
				list(REMOVE_ITEM srcs freertos_smp_v2p0/FreeRTOS-Kernel/portable/MemMang/heap_4.c)
			endif()
	endif()
endif()


armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES bk_common bk_rtos core cmsis driver)


