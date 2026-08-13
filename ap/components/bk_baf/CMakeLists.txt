# Open-source part of BAF: the thin, format-agnostic adapter layer -- a decoder
# facade plus a GPU-compose proxy, both calling the closed core only through its
# public symbols. The public headers (bk_baf.h + bk_baf_types.h) and the
# component-internal header (bk_baf_internal.h) live in this component's own
# include/ dir -- intentionally NOT in the SDK public include tree yet, so the
# API can still change freely while it is not final. include/ is exported so
# dependents (lvgl, the example project, the closed core) reach it via
# 'REQUIRES bk_baf'. The closed-source core (frame decode in baf_decoder.c and
# GPU/CPU compositing in baf_gpu_compose.c / baf_cpu_compose.c) is built as an
# internal library and linked in as libbaf.a.
set(srcs
    src/bk_baf_adapter.c
    src/bk_baf_pacer.c
)

# The adapter binds the closed core's baf_decoder_* exports into its vtable via
# the codec module's decoder interface <modules/baf_decoder.h>, which lives in the
# SDK public include tree (resolved through the 'include' component below); the
# closed symbols themselves resolve at final app link.
armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS include
    PRIV_REQUIRES include avdk_driver
)
