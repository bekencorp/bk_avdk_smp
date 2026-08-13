target_include_directories(platform_s PUBLIC .)
# BEFORE: project partitions_gen.h must beat PROJECT_PARTITIONS_DIR / stub
# copies that share the same filename but different macros.
target_include_directories(platform_s BEFORE PRIVATE ${PARTITION_HDR_DIR})
# boot_param.h (A/B record layout + shared ping-pong algorithm) lives in bl2/;
# the runtime confirm path reuses it. PRIVATE so bl2 headers are not re-exported
# to platform_s dependents.
target_include_directories(platform_s PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../bl2)

set(src
	tfm_hal_isolation.c
	tfm_hal_platform.c
	bk_tfm_ap_power.c
	bk_tfm_ap_shim.c
	bk_tfm_mpc.c
	bk_tfm_ppc.c
	bk_efuse.c
	hal_hw_fih.c
	bk_systick_monitor.c
	hal_sw_fih.c
	bk_sca_defense.c
	boot_param_confirm.c
	# shared A/B back-end (flash ops + CRC32), one copy for BL2 and SPE
	${CMAKE_CURRENT_LIST_DIR}/../bl2/boot_param_ops.c
)

target_sources(platform_s PRIVATE ${src})

# NSC register access gateways for the NS cpu0_app (see reg_nsc.c).
if(CONFIG_REG_ACCESS_NSC)
	target_sources(platform_s PRIVATE reg_nsc.c)
	target_compile_definitions(platform_s PUBLIC CONFIG_REG_ACCESS_NSC)
endif()

# NSC AP boot release gateway (see ap_boot_nsc.c).
if(CONFIG_AP_BOOT_NSC)
	target_sources(platform_s PRIVATE ap_boot_nsc.c)
	target_compile_definitions(platform_s PUBLIC CONFIG_AP_BOOT_NSC)
endif()

target_compile_definitions(platform_s
PUBLIC
	$<$<BOOL:${HW_FIH}>:CONFIG_HW_FIH>
	$<$<BOOL:${TFM_HW_FIH}>:CONFIG_TFM_HW_FIH>
	$<$<BOOL:${TFM_SW_FIH}>:CONFIG_TFM_SW_FIH>
	$<$<BOOL:${SCA_DEFENSE}>:CONFIG_SCA_DEFENSE>
	$<$<BOOL:${CONFIG_SUPPORT_SWD_DEBUG}>:CONFIG_SUPPORT_SWD_DEBUG>
)

if (BL2)
	target_include_directories(platform_bl2 PUBLIC .)

	target_sources(platform_bl2
		PUBLIC
			bk_tfm_ppc.c
			bk_efuse.c
			hal_hw_fih.c
			bk_systick_monitor.c
			hal_sw_fih.c
			bk_sca_defense.c
	)

	if (SOC STREQUAL "bk7236")
		target_sources(platform_bl2
			PUBLIC
				${IDK_PATH}/middleware/driver/efuse/efuse_driver.c
				${IDK_PATH}/middleware/soc/common/hal/efuse_hal.c
		)
	endif()
	target_compile_definitions(platform_bl2
		PUBLIC
			$<$<BOOL:${HW_FIH}>:CONFIG_HW_FIH>
			$<$<BOOL:${BL2_HW_FIH}>:CONFIG_BL2_HW_FIH>
			$<$<BOOL:${BL2_SW_FIH}>:CONFIG_BL2_SW_FIH>
			$<$<BOOL:${SCA_DEFENSE}>:CONFIG_SCA_DEFENSE>
	)
endif()