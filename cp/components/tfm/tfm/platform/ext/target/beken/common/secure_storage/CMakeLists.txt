if (BL2)
	target_include_directories(platform_bl2 PUBLIC .)
	target_sources(platform_bl2
		PRIVATE
			rotpk.c
			otp_flash.c

	)	
endif()

target_include_directories(platform_s PUBLIC .)
target_sources(platform_s 
		PRIVATE 
			rotpk.c
			$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>: otp_flash.c>
			otp_ps.c
			provisioning.c
)