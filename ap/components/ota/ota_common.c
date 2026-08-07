#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "common/bk_err.h"
#include "bk_private/bk_ota_private.h"
#include <soc/soc.h>   /* SOC_FLASH_REG_BASE: applies the S/NS address offset */

#ifdef CONFIG_HTTP_AB_PARTITION
#include "modules/ota.h"
#include "driver/flash_partition.h"
#include "ab_flag.h"
#include "CheckSumUtils.h"
#include "aon_pmu_hal.h"
#endif

/* Flash XIP remap register (bit0: 0=slot A/primary, 1=slot B/secondary), set by
 * MCUboot BL2. Defined unconditionally so the secure-XIP OTA path can read the
 * running slot without CONFIG_HTTP_AB_PARTITION.
 *
 * Use SOC_FLASH_REG_BASE (not a hardcoded 0x44030000) so the S/NS address
 * offset is applied: on the Non-Secure AP (CONFIG_SPE=0) this resolves to the
 * NS alias 0x54030000. A raw read of the secure alias 0x44030000 from the NS
 * AP triggers a Secure/BusFault that hangs the caller (e.g. `ab_version`). */
#define FLASH_BASE_ADDRESS                (SOC_FLASH_REG_BASE)
#define FLASH_OFFSET_ENABLE               (0x19)

#ifdef CONFIG_HTTP_AB_PARTITION
#define FLASH_DEFAULT_VALUE               (0xFFFFFFFF)

/* AON PMU trial reboot counter, shared with the bootloader (driver_ab.c): the
 * count lives in the aon_pmu_r0_t bl2_reset_count field (written via PMU_REG0,
 * read back from PMU_REG7A). The bootloader increments it on every TRIAL boot
 * and rolls back once it reaches try_max; clearing it here on a successful
 * confirm gives each new OTA a full trial budget again (otherwise consecutive
 * OTAs without an intervening cold/NORMAL boot would accumulate the count and
 * spuriously roll back). The field position/width comes from the SoC register
 * definition (aon_pmu_struct.h), so no local bit/mask duplication is needed. */

/* The OTA target slot is always the one opposite the running slot; derive it on
 * demand instead of caching it in a global (the running slot never changes
 * within an OTA session, so this is stable). */
part_flag bk_ota_get_update_partition(void)
{
	return (bk_ota_get_current_partition() == EXEC_B_PART) ? UPDATE_A_PART : UPDATE_B_PART;
}

/* -------------------------------------------------------------------------- */
/* AB ping-pong flag back-end (AP side): SDK flash driver + zlib CRC32.        */
/* See ab_flag.h for the shared record layout and algorithm.                   */
/* -------------------------------------------------------------------------- */

static int ap_flag_read(uint32_t addr, void *buf, uint32_t len)
{
	return bk_flash_read_bytes(addr, (uint8_t *)buf, len);
}

static int ap_flag_erase(uint32_t addr)
{
	return bk_flash_erase_sector(addr);
}

static int ap_flag_write(uint32_t addr, const void *buf, uint32_t len)
{
	return bk_flash_write_bytes(addr, (uint8_t *)buf, len);
}

static uint32_t ap_flag_crc32(const void *buf, uint32_t len)
{
	return crc32_zlib(0u, buf, len);
}

static const ab_flag_ops_t s_ap_ab_ops = {
	.read = ap_flag_read,
	.erase_sector = ap_flag_erase,
	.write = ap_flag_write,
	.crc32 = ap_flag_crc32,
};

static uint32_t ap_flag_partition_base(void)
{
	bk_logic_partition_t *flag_part = bk_flash_partition_get_info(BK_PARTITION_OTA_FINA_EXECUTIVE);

	if (flag_part == NULL) {
		OTA_LOGE("ota_fina_executive partition missing\r\n");
		return 0;
	}
	return flag_part->partition_start_addr;
}

/* Read the freshest valid AB record. @return 0 on success, -1 if none. */
static int ap_ab_record_read(ab_flag_record_t *latest)
{
	uint32_t base = ap_flag_partition_base();

	if (base == 0) {
		return -1;
	}
	return (ab_record_read_latest(base, &s_ap_ab_ops, latest) < 0) ? -1 : 0;
}

/* Commit a fully-populated (semantic fields set) record with flash protection
 * temporarily disabled. */
static void ap_ab_record_commit(ab_flag_record_t *rec)
{
	uint32_t base = ap_flag_partition_base();
	flash_protect_type_t protect_type;

	if (base == 0) {
		return;
	}
	protect_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	(void)ab_record_commit(base, &s_ap_ab_ops, rec);
	bk_flash_set_protect_type(protect_type);
}

/* Build a record from its semantic fields and commit it (magic/seq/crc are
 * stamped by ab_record_commit). The reserved bytes are zeroed so the on-flash
 * layout matches the packager. try_max == 0 falls back to the default.
 * @return the sequence number assigned to the committed record (for logging). */
static uint32_t ap_ab_commit(ab_slot_t exec_slot, ab_slot_t update_slot,
			     uint8_t boot_state, uint8_t dl_state, uint8_t try_max)
{
	ab_flag_record_t rec;

	os_memset(&rec, 0, sizeof(rec));
	rec.exec_slot = (uint8_t)exec_slot;
	rec.update_slot = (uint8_t)update_slot;
	rec.boot_state = boot_state;
	rec.dl_state = dl_state;
	rec.try_max = try_max ? try_max : (uint8_t)AB_FLAG_DEFAULT_TRY_MAX;
	/* The bootloader reboot counter is a 3-bit AON PMU field saturating at 7;
	 * never persist a threshold that count could never reach. */
	if (rec.try_max > 7u) {
		rec.try_max = AB_FLAG_DEFAULT_TRY_MAX;
	}
	ap_ab_record_commit(&rec);
	return rec.seq;
}

/* True if the slot's app partition holds an image, i.e. its first word is not
 * erased (0xFFFFFFFF). A missing partition counts as "no image". */
static int ap_slot_has_image(ab_slot_t slot)
{
	bk_partition_t part = (slot == AB_SLOT_B) ? BK_PARTITION_S_APP : BK_PARTITION_APPLICATION;
	bk_logic_partition_t *info = bk_flash_partition_get_info(part);
	uint32_t head_word = FLASH_DEFAULT_VALUE;

	if (info == NULL) {
		return 0;
	}
	bk_flash_read_bytes(info->partition_start_addr, (uint8_t *)&head_word, sizeof(head_word));
	return (head_word != FLASH_DEFAULT_VALUE);
}

/* Clear the AON PMU trial reboot counter (bits[14:12]) through the AP PMU HAL/LL
 * instead of poking registers directly. The counter is written via PMU_REG0 plus
 * the PMU_REG25 magic latch and read back from PMU_REG7A (aon_pmu_ll_get_r7a_value),
 * which is the authoritative live view of the latched value; aon_pmu_hal_set_r0()
 * stages PMU_REG0 and applies the magic handshake. This mirrors the bootloader's
 * bl_ab_reboot_count_clear() (write R0, read R7A). */
static void ap_ab_reboot_count_clear(void)
{
	aon_pmu_r0_t r;

	r.v = aon_pmu_ll_get_r7a_value();
	r.bl2_reset_count = 0;
	aon_pmu_hal_set_r0(r.v);
}

/* Confirm the currently running trial slot: if the active record is TRIAL and
 * the running slot (remap register) matches update_slot, persist NORMAL with
 * exec_slot = running. Idempotent and AP-only (CP has no OTA component). */
static void ap_ab_try_confirm(void)
{
	ab_flag_record_t rec;
	ab_slot_t running;
	uint32_t seq;

	if (ap_ab_record_read(&rec) != 0) {
		OTA_LOGW("confirm: no valid flag record\r\n");
		return;
	}
	if (rec.boot_state != AB_STATE_TRIAL) {
		OTA_LOGD("confirm: state 0x%x not TRIAL, nothing to do\r\n", rec.boot_state);
		return;
	}

	running = (ab_slot_t)bk_ota_get_current_partition(); /* 0=A, 1=B */
	if (rec.update_slot != (uint8_t)running) {
		/* Running slot is not the trial target (e.g. bootloader rolled back):
		 * do not confirm -- let the current committed state stand. */
		OTA_LOGW("confirm: running %d != update_slot %d, skip\r\n",
			(int)running, (int)rec.update_slot);
		return;
	}

	/* Trial succeeded: commit NORMAL for the running slot, preserving try_max. */
	seq = ap_ab_commit(running, running, (uint8_t)AB_STATE_NORMAL, (uint8_t)AB_DL_IDLE, rec.try_max);

	/* Reset the trial reboot counter so the next OTA starts with a full budget.
	 * Without this, back-to-back OTAs (confirm then re-arm a new TRIAL before any
	 * cold/NORMAL boot clears the count) accumulate the counter until it hits
	 * try_max, and the bootloader then rolls back a perfectly good new image. */
	ap_ab_reboot_count_clear();

	OTA_LOGI("OTA confirmed: exec slot %d (seq->%u)\r\n", (int)running, (unsigned)seq);
}

void bk_ota_double_check_for_execution(void)
{
	/* Lightweight automatic confirm: reaching app init counts as "the trial
	 * image booted". Commits NORMAL for the running slot if a TRIAL is pending
	 * (see ap_ab_try_confirm). AP-only -- CP has no OTA component. */
	OTA_LOGI("bk_ota_double_check_for_execution\r\n");
	ap_ab_try_confirm();
}

#ifdef CONFIG_OTA_HASH_FUNCTION
int32_t ota_do_hash_check(void)
{
	struct ota_rbl_head  rbl_hdr;
	const bk_logic_partition_t *bk_ptr = NULL;
	uint32_t partition_length = 0;
	int ret = BK_FAIL;
	if(bk_ota_get_update_partition() == UPDATE_B_PART) {
		bk_ptr = bk_flash_partition_get_info(BK_PARTITION_S_APP);   //note: when update_partition is B, arg: BK_PARTITION_APPLICATION1,update_partition is A??arg: BK_PARTITION_APPLICATION
	}
	else{    //B-->A
		bk_ptr = bk_flash_partition_get_info(BK_PARTITION_APPLICATION);
	}
	partition_length = bk_flash_partition_get_info(BK_PARTITION_S_APP)->partition_length;
	OTA_LOGD("partition_length :0x%x",partition_length);
	if((bk_ptr == NULL))
	{
		OTA_LOGE(" get %s fail \r\n",bk_ptr->partition_owner);
		return BK_FAIL; 
	}

	ota_get_rbl_head(bk_ptr, &rbl_hdr, partition_length);
	
	ret = ota_hash_verify(bk_ptr, &rbl_hdr);
	if(ret == BK_OK)
	{
		OTA_LOGI("hash sucess!!!! \r\n");
	}

	return ret;
}
#endif

/* Download finished and verified: arm the new image for a trial boot.
 *
 * The image was written to the slot opposite the one currently running, so the
 * committed slot (exec_slot) stays the running slot and the trial slot
 * (update_slot) is its opposite. The bootloader will trial-boot update_slot and
 * roll back to exec_slot if it never confirms. Confirmation happens later, in
 * ap_ab_try_confirm() during the next boot's early init. */
int bk_ota_update_partition_flag(int input_val)
{
	/* Slots are encoded 0=A/1=B (== AB_SLOT_A/AB_SLOT_B), so the running slot is
	 * the committed exec_slot and its bitwise opposite is the trial update_slot. */
	ab_slot_t exec_slot = (ab_slot_t)bk_ota_get_current_partition();
	ab_slot_t update_slot = (ab_slot_t)(exec_slot ^ AB_SLOT_B);
	uint32_t seq;

	(void)input_val;

	seq = ap_ab_commit(exec_slot, update_slot, (uint8_t)AB_STATE_TRIAL, (uint8_t)AB_DL_DONE, 0);
	OTA_LOGI("OTA armed: TRIAL exec=%d update=%d seq->%u\r\n",
		(int)exec_slot, (int)update_slot, (unsigned)seq);
	return BK_OK;
}

/* Test/CLI helper: force the committed exec slot to the opposite of the one
 * currently running (commits NORMAL for the target). Guarded so it refuses to
 * point at an empty slot. */
int bk_ota_swap_execute_partition(void)
{
	ab_slot_t running = (ab_slot_t)bk_ota_get_current_partition();
	ab_slot_t target_slot = (ab_slot_t)(running ^ AB_SLOT_B);
	uint32_t seq;

	/* Both slots must hold an image; swapping onto an empty slot would brick
	 * the device, so refuse unless the target is actually bootable. */
	if (!ap_slot_has_image(AB_SLOT_A) || !ap_slot_has_image(AB_SLOT_B)) {
		OTA_LOGE("only one execute partition and forbid swap! \r\n");
		return BK_FAIL;
	}

	seq = ap_ab_commit(target_slot, target_slot, (uint8_t)AB_STATE_NORMAL, (uint8_t)AB_DL_IDLE, 0);
	OTA_LOGI("swap exec slot %d -> %d (seq->%u)\r\n",
		(int)running, (int)target_slot, (unsigned)seq);
	return BK_OK;
}

uint32 http_get_sapp_partition_length(bk_partition_t partition)
{
	bk_logic_partition_t *bk_ptr = NULL;
	uint32 ret_length;

	bk_ptr = bk_flash_partition_get_info(partition);

	if(NULL == bk_ptr)
	{
		OTA_LOGE("get s_app partition fail! \r\n");
		bk_reboot();
	}

	ret_length = bk_ptr->partition_length;

	return ret_length;
}

#endif // CONFIG_HTTP_AB_PARTITION

/* HW XIP remap accessor (bit0: 0=A, 1=B), compiled unconditionally so the
 * secure-XIP OTA backend can read the running slot. */
static uint8 ota_get_flash_offset_enable_value(void)
{
	uint8 ret_val;

	ret_val = (REG_READ((FLASH_BASE_ADDRESS + FLASH_OFFSET_ENABLE*4)) & 0x1);
	OTA_LOGI("ret_val  :0x%x\r\n",ret_val);

	return ret_val;
}

uint8 bk_ota_get_current_partition(void)
{
	return ota_get_flash_offset_enable_value();  //0x0: slot A, 0x1: slot B
}

#if CONFIG_OTA_DISPLAY_PICTURE_DEMO
#include "bk_partition.h"
static char s_device_id[128] = {0};
static int bk_sconf_trans_stop(void)
{
    int ret = BK_OK;

    ret = bk_sconf_get_channel_name(s_device_id);
    if ((ret == 0) && (os_strlen(s_device_id) > 0))
	{
        #if CONFIG_BK_NETWORK_TRANSFER
        ntwk_trans_stop(s_device_id);
        #endif
    }
	
	return ret;
}

int bk_sconf_trans_start(void)
{
    int ret = BK_OK;

    if ((ret == 0) && (os_strlen(s_device_id) > 0))
	{
        #if CONFIG_BK_NETWORK_TRANSFER
        ret = ntwk_trans_start(s_device_id);
        #endif
    }

    return ret;
}

int ota_update_with_display_open(void)
{
	int ret = BK_OK;

    ret = bk_sconf_trans_stop();
    if(ret != BK_OK)
    {
        OTA_LOGE("stop transfer fail! \r\n");
    }
#if CONFIG_DUAL_SCREEN_AVI_PLAYER
    bk_dual_screen_avi_player_stop();
#endif
	audio_engine_deinit();
	bk_ota_display_init();	
	if(bk_ota_image_display_open(PATH_SD_FILE("/ota_image.jpg")) != BK_OK)
	{
		OTA_LOGE("open disp failed. \r\n");
		bk_ota_display_deinit();
		ret = BK_FAIL;
	}

	return ret;
}

int ota_update_with_display_close(void)
{
	int ret = BK_FAIL;

	if(bk_ota_image_display_close() != BK_OK)
	{
		OTA_LOGE("close disp failed. \r\n");
		ret = BK_FAIL;
	}

	bk_ota_display_deinit();

	return ret;
}
#endif

static ota_event_callback_t s_ota_event_callback = NULL;

int ota_event_callback_register(ota_event_callback_t callback)
{
	s_ota_event_callback = callback;

	return 0;
}

int ota_input_event_handler(evt_ota event_param)
{
	if(NULL != s_ota_event_callback)
	{
		s_ota_event_callback(event_param);
	}

	return 0;
}

static ota_process_data_callback_t s_ota_data_process = NULL;

void register_ota_callback(ota_process_data_callback_t ota_callback)
{
	s_ota_data_process = ota_callback;
}

int bk_ota_process_data(char*receive_data, uint32_t len, uint32_t received, uint32_t total)
{
	if(s_ota_data_process != NULL)
	{
		/* Propagate the write result so the HTTP layer can abort the transfer
		 * on a flash-write failure instead of streaming the rest of the image. */
		return s_ota_data_process(receive_data, len, received, total);
	}
	return 0;
}

int ota_extract_path_segment(char *in_name, char *out_name) 
{
	int ret = BK_FAIL;
	char *last_token = NULL;
	char *token = strtok(in_name, "/");

	/*Split the string: Use the'/'character as a delimiter*/
	while (token != NULL) {
		last_token = token;  /*record the last token*/
		token = strtok(NULL, "/");
	}

	if(last_token != NULL) {
		/*Keep the file extension and directly copy the last token”.*/
		strncpy(out_name, last_token, strlen(last_token));
		out_name[strlen(last_token)] = '\0';
		ret = BK_OK;
	} else {
		OTA_LOGE(".string last_token is NULL\n");
	}

	return ret;
}