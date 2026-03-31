armino_build_get_property(target ARMINO_SOC)

set(incs)
set(srcs)

list(APPEND incs
	./
	./interrupt_controller
)


list(APPEND srcs
	core_init.c
	./interrupt_controller/interrupt_controller.c
)

if(CONFIG_ARM_CORE_STAR)
list(APPEND srcs
	star_irq.c
)
elseif(CONFIG_ARM_CORE_CM52)
list(APPEND srcs
	cm52_irq.c
)
elseif(CONFIG_ARM_CORE_CM55)
list(APPEND srcs
	cm55_irq.c
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES coredump os_source)

