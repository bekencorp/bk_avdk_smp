target_include_directories(platform_s PUBLIC .)
target_sources(platform_s 
		PRIVATE 
			mpu_armv8m_drv.c
)