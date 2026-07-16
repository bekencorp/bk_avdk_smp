#pragma once

#include <stdint.h>
#include "vcdec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque standalone PP instance handle. */
typedef void *vcdec_pp_handle;

/**
 * @brief Standalone PP instance configuration.
 */
typedef struct vcdec_pp_process_config_t {
	uint32_t            timeout_ms;   /* HW wait timeout, ms */
	vcdec_frame_done_cb done_cb;      /* optional, invoked after each operation */
	void               *args;         /* user context passed to done_cb */
} vcdec_pp_process_config_t;

/**
 * @brief One standalone PP processing request.
 *
 * Input is NV12 (YCbCr 4:2:0 semiplanar): a luminance plane at in_y_bus and an
 * interleaved CbCr plane at in_c_bus. Output is written to out_buffer in the
 * requested out_format (NV12 / RGB565 / RGB888). out_width / out_height select
 * the output resolution; zero means same as the corresponding input dimension.
 *
 * All buffers must be reachable by the PP bus master (e.g. PSRAM) and meet the
 * hardware alignment requirement.
 */
typedef struct vcdec_pp_process_req_t {
	uint32_t              in_y_bus;    /* NV12 Y plane bus address */
	uint32_t              in_c_bus;    /* NV12 interleaved CbCr plane bus address */
	uint16_t              in_width;    /* input picture width in pixels */
	uint16_t              in_height;   /* input picture height in pixels */
	uint16_t              out_width;   /* output width (0 -> = in_width) */
	uint16_t              out_height;  /* output height (0 -> = in_height) */
	void                 *out_buffer;  /* output buffer (NV12 / RGB565 / RGB888) */
	uint32_t              out_size;    /* output buffer size in bytes */
	vcdec_pp_out_format_e out_format;  /* VCDEC_PP_OUT_NV12 / RGB565 / RGB888 */
} vcdec_pp_process_req_t;

/**
 * @brief Create a standalone PP instance (allocates handle, inits completion sem).
 */
vcdec_ret_e vcdec_pp_process_init(vcdec_pp_handle *handle_p, vcdec_pp_process_config_t *config);

/**
 * @brief Run one blocking PP operation (NV12 scale and/or format conversion).
 *
 * The PP HW power domain and the PP interrupt must already be enabled by the
 * caller (the bk_vpu wrapper does this via the hw decoder controller).
 */
vcdec_ret_e vcdec_pp_process(vcdec_pp_handle handle, vcdec_pp_process_req_t *req);

/**
 * @brief Destroy a standalone PP instance.
 */
void vcdec_pp_process_deinit(vcdec_pp_handle handle);

#ifdef __cplusplus
}
#endif
