if (CONFIG_HIGH_PERFORMANCE_DMA)
	list(APPEND incs hp_dma)

	list(APPEND srcs
		hp_dma/hpdma_driver.c
	)

	if (CONFIG_HIGH_PERFORMANCE_DMA_TEST)
		list(APPEND srcs hp_dma/hpdma_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()

