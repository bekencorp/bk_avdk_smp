if (BL2)
	target_include_directories(platform_bl2 PUBLIC .)
	target_sources(platform_bl2
		PRIVATE
			rotpk.c
			otp_flash.c

	)
	
endif()