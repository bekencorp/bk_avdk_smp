set(incs
	$ENV{PARTITIONS_DIR}
)
set(srcs
	$ENV{PARTITIONS_DIR}/flash_partitions_table.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_cli bk_common bk_log)
