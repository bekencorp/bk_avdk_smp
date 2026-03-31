set(incs include src)
set(srcs "")
set(priv_req bk_common)

if (CONFIG_FRAME_BUFFER)
	list(APPEND srcs
		src/bk_mem_slab.c
		src/bk_mem_sram.c
		src/bk_frame_buffer.c
	)
endif()

list(APPEND priv_req unity include avdk_utils)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
