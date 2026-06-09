#include <os/os.h>
#include <os/mem.h>
#include <components/avdk_utils/avdk_check.h>
#include <common/avdk_pixel_types.h>
#include <common/bk_err.h>

#include "components/bk_encode/bk_jpeg_encode_ctlr.h"
#include "hw_encoder_ctlr.h"
#include "private_jpeg_encode_ctlr.h"

#define TAG "bk_jpeg_enc_hwf"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static vcenc_input_e jpeg_hw_flexa_map_input_format(uint32_t pixel_fmt)
{
	if (pixel_fmt == (uint32_t)BK_PIXEL_FORMAT_NV12)
		return VCENC_INPUT_NV12;
	return VCENC_INPUT_NV12;
}

static avdk_err_t jpeg_hw_flexa_vcenc_ret_to_avdk(vcenc_ret_e r)
{
	if (r == VCENC_FRAME_READY || r == VCENC_OK)
		return AVDK_ERR_OK;
	if (r == VCENC_HW_TIMEOUT)
		return AVDK_ERR_TIMEOUT;
	if (r == VCENC_NULL_ARGUMENT || r == VCENC_INVALID_ARGUMENT)
		return AVDK_ERR_INVAL;
	if (r == VCENC_MEMORY_ERROR || r == VCENC_EWL_MEMORY_ERROR)
		return AVDK_ERR_NOMEM;
	return AVDK_ERR_HWERROR;
}

static void jpeg_hw_flexa_complete(private_jpeg_encode_hw_flexa_ctlr_t *ctrl,
				   void *buffer, uint32_t length, uint32_t type, uint32_t status)
{
	if (ctrl == NULL)
		return;

	if (ctrl->config.outbuf_complete != NULL) {
		bk_jpeg_encode_outbuf_info_t info = {
			.outbuf = buffer,
			.length = length,
			.type = type,
			.status = status,
			.sequence = 0,
			.args = ctrl->config.outbuf_complete_args,
		};
		ctrl->config.outbuf_complete(&info);
	}
	if (buffer == (void *)(uintptr_t)ctrl->frame_cfg.out_buffer)
		ctrl->frame_cfg.out_buffer = 0;
}

static void jpeg_hw_flexa_done_cb(void *buffer, uint32_t length, uint32_t type,
				  uint32_t status, uint32_t args)
{
	private_jpeg_encode_hw_flexa_ctlr_t *ctrl =
		(private_jpeg_encode_hw_flexa_ctlr_t *)(uintptr_t)args;

	if (ctrl == NULL)
		return;

	ctrl->encode_result = (status == VCENC_OK) ? BK_OK : BK_FAIL;
	jpeg_hw_flexa_complete(ctrl, buffer, length, type, ctrl->encode_result);
}

static avdk_err_t jpeg_hw_flexa_msg_callback(void *param)
{
	private_jpeg_encode_hw_flexa_ctlr_t *ctrl =
		(private_jpeg_encode_hw_flexa_ctlr_t *)param;

	if (ctrl == NULL)
		return AVDK_ERR_INVAL;

	ctrl->last_ret = vcenc_jpeg_encode_frame(ctrl->handle, &ctrl->frame_cfg);
	if (ctrl->bond != NULL && ctrl->bond->frame_done != NULL) {
		ctrl->bond->frame_done(jpeg_hw_flexa_vcenc_ret_to_avdk(ctrl->last_ret) == AVDK_ERR_OK ?
				       BK_OK : BK_FAIL,
				       ctrl->bond);
	}
	return jpeg_hw_flexa_vcenc_ret_to_avdk(ctrl->last_ret);
}

static void jpeg_hw_flexa_encoder_entry(void *arg)
{
	private_jpeg_encode_hw_flexa_ctlr_t *ctrl =
		(private_jpeg_encode_hw_flexa_ctlr_t *)arg;

	if (ctrl == NULL) {
		LOGE("encoder thread started with NULL context\r\n");
		return;
	}

	rtos_set_semaphore(&ctrl->sem);

	while (ctrl->enc_status) {
		rtos_get_semaphore(&ctrl->enc_start_sem, BEKEN_WAIT_FOREVER);
		if (!ctrl->enc_status)
			break;

		if (ctrl->frame_cfg.out_buffer == 0 && ctrl->config.outbuf_malloc != NULL) {
			void *out = ctrl->config.outbuf_malloc(CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER,
							       ctrl->config.outbuf_malloc_args);
			if (out != NULL) {
				ctrl->frame_cfg.out_buffer = (uint32_t)(uintptr_t)out;
				ctrl->frame_cfg.out_len = CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER;
			}
		}

		if (ctrl->frame_cfg.out_buffer == 0 || ctrl->frame_cfg.in_buffer == 0) {
			LOGE("invalid input/output buffer\r\n");
			jpeg_hw_flexa_complete(ctrl, (void *)(uintptr_t)ctrl->frame_cfg.out_buffer,
					       0, 0, BK_FAIL);
			if (ctrl->bond != NULL && ctrl->bond->frame_done != NULL)
				ctrl->bond->frame_done(BK_FAIL, ctrl->bond);
			continue;
		}

		hw_encoder_msg_t msg = {
			.type = HW_ENCODER_MSG_ENCODE,
			.encoder_type = HW_ENCODER_TYPE_JPEG,
			.callback = jpeg_hw_flexa_msg_callback,
			.param = ctrl,
			.sem = &ctrl->enc_done_sem,
		};

		avdk_err_t ret = hw_encoder_send_msg(&msg, BEKEN_WAIT_FOREVER);
		if (ret != AVDK_ERR_OK) {
			LOGE("hw_encoder_send_msg failed: %d\r\n", ret);
			jpeg_hw_flexa_complete(ctrl, (void *)(uintptr_t)ctrl->frame_cfg.out_buffer,
					       0, 0, BK_FAIL);
			if (ctrl->bond != NULL && ctrl->bond->frame_done != NULL)
				ctrl->bond->frame_done(BK_FAIL, ctrl->bond);
			continue;
		}

		ret = rtos_get_semaphore(&ctrl->enc_done_sem, 3000);
		if (ret != BK_OK) {
			LOGE("wait encode done failed: %d\r\n", ret);
			(void)vcenc_jpeg_abort(ctrl->handle);
			jpeg_hw_flexa_complete(ctrl, (void *)(uintptr_t)ctrl->frame_cfg.out_buffer,
					       0, 0, BK_FAIL);
			if (ctrl->bond != NULL && ctrl->bond->frame_done != NULL)
				ctrl->bond->frame_done(BK_FAIL, ctrl->bond);
		}
	}

	if (ctrl->frame_cfg.out_buffer != 0)
		jpeg_hw_flexa_complete(ctrl, (void *)(uintptr_t)ctrl->frame_cfg.out_buffer, 0, 0, BK_FAIL);

	rtos_set_semaphore(&ctrl->sem);
	rtos_delete_thread(NULL);
}

static avdk_err_t jpeg_hw_flexa_ctlr_init(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_hw_flexa_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_hw_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	avdk_err_t ret = hw_encoder_register(HW_ENCODER_TYPE_JPEG, control);
	if (ret != AVDK_ERR_OK)
		return ret;

	if (rtos_init_semaphore(&control->sem, 1) != BK_OK ||
	    rtos_init_semaphore(&control->enc_start_sem, 1) != BK_OK ||
	    rtos_init_semaphore(&control->enc_done_sem, 1) != BK_OK) {
		(void)hw_encoder_unregister(control);
		return AVDK_ERR_GENERIC;
	}

	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_hw_flexa_ctlr_open(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_hw_flexa_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_hw_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	vcenc_config_t      common_cfg = {
		.mode          = VCENC_HW_SLICE_MODE,
		.timeout_ms    = 0,
		.frame_done_cb = jpeg_hw_flexa_done_cb,
		.slice_done_cb = NULL,
		.args          = control,
	};
	vcenc_jpeg_config_t jpeg_cfg = {
		.width                       = (uint16_t)control->config.width,
		.height                      = (uint16_t)control->config.height,
		.in_type                     = jpeg_hw_flexa_map_input_format(control->config.input_format),
		.input_linebuf_depth         = 1,
		.input_linebuf_loopback_en   = 1,
		.input_linebuf_hw_mode_en    = 1,
		.amount_per_loopback         = control->config.input_flexa_cnt ?
						  control->config.input_flexa_cnt : 1U,
	};

	os_memset(&control->frame_cfg, 0, sizeof(control->frame_cfg));
	control->frame_cfg.width      = (uint16_t)control->config.width;
	control->frame_cfg.height     = (uint16_t)control->config.height;
	control->frame_cfg.in_buffer  = control->config.input_buf;
	control->frame_cfg.in_lines   = control->config.input_size;
	control->frame_cfg.quality    = control->config.quality;

	control->handle = NULL;
	vcenc_ret_e jr = vcenc_jpeg_init(&control->handle, &common_cfg, &jpeg_cfg);
	if (jr != VCENC_OK)
		return jpeg_hw_flexa_vcenc_ret_to_avdk(jr);

	jr = vcenc_jpeg_memalloc_register(control->handle, hw_encoder_malloc, hw_encoder_free);
	if (jr != VCENC_OK) {
		(void)vcenc_jpeg_deinit(control->handle);
		control->handle = NULL;
		return jpeg_hw_flexa_vcenc_ret_to_avdk(jr);
	}

	jr = vcenc_jpeg_open(control->handle);
	if (jr != VCENC_OK) {
		(void)vcenc_jpeg_deinit(control->handle);
		control->handle = NULL;
		return jpeg_hw_flexa_vcenc_ret_to_avdk(jr);
	}

	control->enc_status = 1;
	control->opened = 1;

	bk_err_t tr = rtos_create_hsram_thread(&control->thread,
					       BEKEN_DEFAULT_WORKER_PRIORITY,
					       "jpeg_hw_flexa",
					       (beken_thread_function_t)jpeg_hw_flexa_encoder_entry,
					       CONFIG_BK_ENCODER_H264_TASK_SIZE,
					       control);
	if (tr != BK_OK) {
		control->enc_status = 0;
		control->opened = 0;
		(void)vcenc_jpeg_close(control->handle);
		(void)vcenc_jpeg_deinit(control->handle);
		return AVDK_ERR_GENERIC;
	}

	rtos_get_semaphore(&control->sem, BEKEN_WAIT_FOREVER);
	LOGI("JPEG hw flexa opened %ux%u\r\n", control->config.width, control->config.height);
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_hw_flexa_ctlr_close(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_hw_flexa_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_hw_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	control->enc_status = 0;
	rtos_set_semaphore(&control->enc_start_sem);
	rtos_get_semaphore(&control->sem, BEKEN_WAIT_FOREVER);

	if (control->opened) {
		(void)vcenc_jpeg_close(control->handle);
		(void)vcenc_jpeg_deinit(control->handle);
		control->opened = 0;
	}
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_hw_flexa_ctlr_deinit(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_hw_flexa_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_hw_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	rtos_deinit_semaphore(&control->sem);
	rtos_deinit_semaphore(&control->enc_start_sem);
	rtos_deinit_semaphore(&control->enc_done_sem);
	return hw_encoder_unregister(control);
}

static avdk_err_t jpeg_hw_flexa_ctlr_ioctl(bk_jpeg_encode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
	private_jpeg_encode_hw_flexa_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_hw_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	switch ((bk_jpeg_encode_ioctl_cmd_t)cmd) {
	case BK_JPEG_ENCODE_IOCTL_SET_QUALITY:
		if (arg == NULL)
			return AVDK_ERR_INVAL;
		control->config.quality = *(uint8_t *)arg;
		control->frame_cfg.quality = control->config.quality;
		return AVDK_ERR_OK;
	case BK_JPEG_ENCODE_IOCTL_SET_FRAME_READY:
		rtos_set_semaphore(&control->enc_start_sem);
		return AVDK_ERR_OK;
	case BK_JPEG_ENCODE_IOCTL_REGISTER_BOND:
		control->bond = (bk_flexa_bond_t *)arg;
		return control->bond ? AVDK_ERR_OK : AVDK_ERR_INVAL;
	case BK_JPEG_ENCODE_IOCTL_UNREGISTER_BOND:
		if (control->bond == (bk_flexa_bond_t *)arg) {
			control->bond = NULL;
			return AVDK_ERR_OK;
		}
		return AVDK_ERR_INVAL;
	case BK_JPEG_ENCODE_IOCTL_STOP_ENCODE:
		return jpeg_hw_flexa_vcenc_ret_to_avdk(vcenc_jpeg_abort(control->handle));
	default:
		return AVDK_ERR_UNSUPPORTED;
	}
}

static avdk_err_t jpeg_hw_flexa_ctlr_del(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_hw_flexa_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_hw_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
	os_free(control);
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_hw_flexa_ctlr_encode_frame(bk_jpeg_encode_ctlr_handle_t handle,
						  bk_jpeg_encode_input_t *input)
{
	private_jpeg_encode_hw_flexa_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_hw_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (input != NULL) {
		if (input->pic_buf)
			control->frame_cfg.in_buffer = input->pic_buf;
		if (input->pic_lines)
			control->frame_cfg.in_lines = input->pic_lines;
		if (input->out_buf) {
			control->frame_cfg.out_buffer = input->out_buf;
			control->frame_cfg.out_len = input->out_size;
		}
	}

	rtos_set_semaphore(&control->enc_start_sem);
	return AVDK_ERR_OK;
}

avdk_err_t bk_jpeg_encode_hw_flexa_ctlr_new(bk_jpeg_encode_ctlr_handle_t *handle,
					    bk_jpeg_encode_hw_flexa_config_t *config)
{
	AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, "invalid args");
	AVDK_RETURN_ON_FALSE(config->width && config->height, AVDK_ERR_INVAL, TAG, "invalid size");

	private_jpeg_encode_hw_flexa_ctlr_t *ctrl =
		(private_jpeg_encode_hw_flexa_ctlr_t *)os_malloc(sizeof(*ctrl));
	if (ctrl == NULL)
		return AVDK_ERR_NOMEM;

	os_memset(ctrl, 0, sizeof(*ctrl));
	os_memcpy(&ctrl->config, config, sizeof(*config));
	ctrl->ops.init = jpeg_hw_flexa_ctlr_init;
	ctrl->ops.open = jpeg_hw_flexa_ctlr_open;
	ctrl->ops.encode_frame = jpeg_hw_flexa_ctlr_encode_frame;
	ctrl->ops.close = jpeg_hw_flexa_ctlr_close;
	ctrl->ops.deinit = jpeg_hw_flexa_ctlr_deinit;
	ctrl->ops.ioctl = jpeg_hw_flexa_ctlr_ioctl;
	ctrl->ops.del = jpeg_hw_flexa_ctlr_del;

	*handle = &ctrl->ops;
	return AVDK_ERR_OK;
}
