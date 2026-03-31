if (CONFIG_GENERAL_DMA)
	list(APPEND incs general_dma)

	if (CONFIG_GENERAL_DMA_V2P6)
		list(APPEND incs general_dma/v2p6)
		list(APPEND srcs
			general_dma/v2p6/dma_driver.c
		)
	elseif (CONFIG_GENERAL_DMA_V2PX)
		list(APPEND incs general_dma/v2px)
		list(APPEND srcs
			general_dma/v2px/dma_driver.c
		)
	endif()

	if (CONFIG_GENERAL_DMA_TEST)
		list(APPEND srcs general_dma/dma_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
