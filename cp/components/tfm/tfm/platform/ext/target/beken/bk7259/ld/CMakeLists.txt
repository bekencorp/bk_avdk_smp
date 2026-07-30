# Phase-1 uses Isolation Level 2 -> use the plain secure linker script.
# For L3, switch to tfm_isolation_l3.ld (kept in this dir for the evolution).
target_add_scatter_file(tfm_s
        $<$<AND:$<VERSION_LESS:${TFM_ISOLATION_LEVEL},3>,$<C_COMPILER_ID:GNU>>:bk7259_s.ld>
        $<$<AND:$<VERSION_EQUAL:${TFM_ISOLATION_LEVEL},3>,$<C_COMPILER_ID:GNU>>:tfm_isolation_l3.ld>
)
if (BL2)
        target_add_scatter_file(bl2 $<$<C_COMPILER_ID:GNU>:bk7259_bl2.ld>)
endif()
