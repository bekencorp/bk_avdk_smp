set(incs include)
set(srcs )
set(priv_incs )

if (CONFIG_BK_JERRYSCRIPT)
	file(GLOB_RECURSE jerry_srcs "${CMAKE_CURRENT_LIST_DIR}/jerryscript/jerry-core/*.c")

	list(APPEND srcs
		${jerry_srcs}
		src/bk_js_port.c
		src/bk_js_runtime.c
		src/bk_js_app.c
		src/bk_js_fs.c
	)

	list(APPEND priv_incs
		jerryscript/jerry-core/include
		jerryscript/jerry-core
		jerryscript/jerry-core/api
		jerryscript/jerry-core/debugger
		jerryscript/jerry-core/ecma/base
		jerryscript/jerry-core/ecma/builtin-objects
		jerryscript/jerry-core/ecma/builtin-objects/typedarray
		jerryscript/jerry-core/ecma/operations
		jerryscript/jerry-core/jcontext
		jerryscript/jerry-core/jmem
		jerryscript/jerry-core/jrt
		jerryscript/jerry-core/lit
		jerryscript/jerry-core/parser/js
		jerryscript/jerry-core/parser/regexp
		jerryscript/jerry-core/vm
	)

	if (CONFIG_BK_JERRYSCRIPT_UI)
		list(APPEND srcs
			src/bk_js_lv.c
		)
	endif()

	if (CONFIG_BK_JERRYSCRIPT_CLI)
		list(APPEND srcs
			src/bk_js_cli.c
		)
	endif()
endif()

set(js_priv_requires bk_common bk_rtos bk_system bk_cli bk_vfs lvgl)

armino_component_register(SRCS "${srcs}"
        INCLUDE_DIRS "${incs}"
        PRIV_INCLUDE_DIRS "${priv_incs}"
        PRIV_REQUIRES ${js_priv_requires}
        )

if (CONFIG_BK_JERRYSCRIPT)

component_compile_options(
	-Wno-error
	-Wno-attributes
)

component_compile_definitions(
	JERRY_EXTERNAL_CONTEXT=1
	JERRY_ERROR_MESSAGES=1
	JERRY_LINE_INFO=1
	JERRY_VM_HALT=1
	JERRY_MEM_STATS=1
	JERRY_STACK_LIMIT=6
)

endif()
