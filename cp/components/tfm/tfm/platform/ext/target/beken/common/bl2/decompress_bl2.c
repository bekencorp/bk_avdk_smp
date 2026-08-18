#include <inttypes.h>
#include <driver/flash.h>
#include "partitions_gen.h"
#include <stdlib.h>
#include <string.h>

#define TAG "decompress"
#define BL2_DECOMPRESS_LOGD BK_LOGD
#define BL2_DECOMPRESS_LOGI BK_LOGI
#define BL2_DECOMPRESS_LOGW BK_LOGW
#define BL2_DECOMPRESS_LOGE BK_LOGE
#define BL2_DECOMPRESS_DEBUG 0

#define TOVIRTURE(addr) ((addr)%34+(addr)/34*32)
#define TOPHY(addr) ((addr)%32+(addr)/32*34)
#define CEIL_ALIGN_34(addr) (((addr) + 34 - 1) / 34 * 34)
#define ALIGN_4096(addr) (((addr) + 4096 - 1) / 4096 * 4096)
#define COMPRESS_BLOCK_SIZE (64*1024)

extern uint8_t *decompress_in_memory();
extern uint32_t get_flash_map_offset(uint32_t index);
extern uint32_t get_flash_map_size(uint32_t index);
extern uint32_t get_flash_map_phy_size(uint32_t index);

typedef struct {
	uint8_t crc;
} CRC8_Context;

typedef struct {
	uint8_t index;
	uint8_t data[33];
	CRC8_Context crc8;
} resume_block_t;

static uint8_t UpdateCRC8(uint8_t crcIn, uint8_t byte)
{
	uint8_t crc = crcIn;
	uint8_t i;

	crc ^= byte;

	for (i = 0; i < 8; i++) {
		if (crc & 0x01) {
			crc = (crc >> 1) ^ 0x8C;
		} else {
			crc >>= 1;
		}
	}
	return crc;
}

static void CRC8_Init( CRC8_Context *inContext )
{
    inContext->crc = 0;
}

static void CRC8_Update( CRC8_Context *inContext, const void *inSrc, size_t inLen )
{
	const uint8_t *src = (const uint8_t *) inSrc;
	const uint8_t *srcEnd = src + inLen;
	while ( src < srcEnd ) {
		inContext->crc = UpdateCRC8(inContext->crc, *src++);
	}
}

static void CRC8_Final( CRC8_Context *inContext, uint8_t *outResult )
{
	*outResult = inContext->crc & 0xffu;
}

static uint32_t get_resume_base_address(void)
{
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);
	uint32_t area_size = get_flash_map_phy_size(0);
	uint32_t back_address = primary_all_phy_offset + area_size - 4096;
	return back_address;
}


static uint8_t read_resume_block(uint32_t back_address, uint8_t* resume_data, size_t resume_data_size)
{
	resume_block_t resume_block[2];
	CRC8_Context crc_8;
	uint8_t idx = 0;
	for(; idx < 4096/35; ++idx) {
		CRC8_Init(&crc_8);
		resume_block_t* curr = &resume_block[idx % 2];
		memset(curr,0xFF,sizeof(resume_block_t));
		bk_flash_read_bytes(back_address + idx * sizeof(resume_block_t), (uint8_t*)curr, sizeof(resume_block_t));
		if(curr->index != 0xFF){
			CRC8_Update(&crc_8, &curr->index, sizeof(curr->index));
			CRC8_Update(&crc_8, curr->data, sizeof(curr->data));
			if(crc_8.crc != curr->crc8.crc){
				BL2_DECOMPRESS_LOGE(TAG, "resume crc8 error!\r\n");
				return 0xffu;
			}
		} else {
			break;
		}
	}
	memcpy(resume_data,resume_block[(idx + 1) % 2].data,resume_data_size);
	return idx;
}

static uint8_t write_resume_block(uint8_t idx, uint32_t back_address, uint8_t* resume_data, size_t resume_data_size)
{
	resume_block_t resume_block;
	memset(&resume_block,0xFF,sizeof(resume_block));
	memcpy(resume_block.data,resume_data,resume_data_size);
	CRC8_Context crc_8;
	CRC8_Init(&crc_8);
	CRC8_Update(&crc_8, &idx, sizeof(idx));
	CRC8_Update(&crc_8, resume_block.data, sizeof(resume_block.data));
	resume_block.index = idx;
	resume_block.crc8 = crc_8;
	bk_flash_write_bytes(back_address + idx * sizeof(resume_block), (uint8_t*)&resume_block, sizeof(resume_block));
}

static uint32_t idx_sum(uint16_t* buffer, size_t idx)
{
	uint32_t sum = 0;
	for( size_t i = 0; i < idx; ++i){
		sum += buffer[i];
	}
	return sum;
}

static uint32_t resume_flash(uint32_t block_num)
{
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);
	uint32_t area_size = get_flash_map_phy_size(0);

	uint32_t back_address = primary_all_phy_offset + area_size - 4096;
	uint32_t back_data_size = CEIL_ALIGN_34(primary_all_phy_offset) - primary_all_phy_offset; // COMPRESS_BLOCK = 68k
	uint8_t back_data[back_data_size];
	uint8_t restart_block_idx = read_resume_block(back_address, back_data, back_data_size);

	if(restart_block_idx == 0 || restart_block_idx == 0xffu) {
		flash_area_erase_fast(primary_all_phy_offset, area_size);
		restart_block_idx = 0;
	}
	uint32_t restart_block_offset = primary_all_phy_offset + TOPHY(COMPRESS_BLOCK_SIZE) * restart_block_idx;
	if(restart_block_idx < block_num) {
		flash_area_erase_fast(restart_block_offset, TOPHY(COMPRESS_BLOCK_SIZE) + 4 * 1024);
		bk_flash_write_bytes(restart_block_offset, back_data, back_data_size);
	} else if(restart_block_idx == block_num) {
		uint32_t erase_size = back_address - restart_block_offset;
		flash_area_erase_fast(restart_block_offset, erase_size);
		bk_flash_write_bytes(restart_block_offset, back_data, back_data_size);
	}

	return restart_block_idx;
}

static void back_flash(uint32_t restart_block_idx)
{
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);
	uint32_t area_size = get_flash_map_phy_size(0);

	uint32_t back_address = primary_all_phy_offset + area_size - 4096;
	uint32_t back_data_size = CEIL_ALIGN_34(primary_all_phy_offset) - primary_all_phy_offset; // COMPRESS_BLOCK = 68k
	uint8_t back_data[back_data_size];
	uint32_t restart_block_offset = primary_all_phy_offset + TOPHY(COMPRESS_BLOCK_SIZE) * (restart_block_idx + 1);
	bk_flash_read_bytes(restart_block_offset, back_data, back_data_size);
	write_resume_block(restart_block_idx, back_address, back_data, back_data_size);
}

int
boot_copy_region(struct boot_loader_state *state,
				 const struct flash_area *fap_src,
				 const struct flash_area *fap_dst,
				 uint32_t off_src, uint32_t off_dst, uint32_t sz)
{
	uint32_t bytes_copied = 0;
	uint32_t primary_all_vir_size = get_flash_map_size(0);
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);
	uint32_t ota_phy_size = get_flash_map_phy_size(1);
	uint32_t block_num = (primary_all_vir_size) / COMPRESS_BLOCK_SIZE;
	uint16_t block_list[block_num + 2];
	uint8_t* buf;
	uint8_t* decode_buf;

	uint8_t restart_block_idx;
	restart_block_idx = resume_flash(block_num);

	int rate_process = block_num / 5;
	uint32_t vir_primary_all_start_address = TOVIRTURE(CEIL_ALIGN_34(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET));

	uint8_t block_idx = 0;

	bytes_copied = BL2_HEADER_SIZE;
	flash_area_read(fap_src, off_src + bytes_copied, block_list, 2 * (block_num + 2));
	bytes_copied += 2 * (block_num + 2);

	bytes_copied += idx_sum(block_list, restart_block_idx);

	for (block_idx = restart_block_idx; block_idx < block_num; block_idx++){
		buf = (uint8_t*)malloc(block_list[block_idx]);
		if(buf == NULL){
			BL2_DECOMPRESS_LOGE(TAG, "memory malloc fail\r\n");
			return -1;
		}
		flash_area_read(fap_src, off_src + bytes_copied, buf, block_list[block_idx]);

		decode_buf = decompress_in_memory(buf, COMPRESS_BLOCK_SIZE, 0);
		bk_flash_write_cbus(vir_primary_all_start_address + COMPRESS_BLOCK_SIZE * block_idx, (decode_buf), COMPRESS_BLOCK_SIZE);
		back_flash(restart_block_idx);
		restart_block_idx += 1;

		free(buf);
		buf = NULL;
		free(decode_buf);

		bytes_copied += block_list[block_idx];
		if(block_idx % rate_process == 0) {
			BL2_DECOMPRESS_LOGI(TAG, "OTA %d%%\r\n",(block_idx / rate_process + 1) * 20);
		}

	}

	uint16_t last_block_before_size = block_list[block_idx+1];
	uint16_t last_block_after_size  = block_list[block_idx];
	buf = (uint8_t*)malloc(last_block_after_size);
	if(buf == NULL){
		BL2_DECOMPRESS_LOGE(TAG, "memory malloc fail\r\n");
		return -1;
	}
	flash_area_read(fap_src, off_src + bytes_copied, buf, last_block_after_size);

	decode_buf = decompress_in_memory(buf, last_block_before_size, 0);

	uint16_t write_size = (last_block_before_size + 31) / 32 * 32;

	bk_flash_write_cbus(vir_primary_all_start_address + COMPRESS_BLOCK_SIZE * block_idx, (decode_buf), write_size);

	free(decode_buf);
	decode_buf = NULL;
	free(buf);
	buf = NULL;

	uint32_t ota_phy_offset = get_flash_map_offset(1);
	flash_area_erase_fast(get_resume_base_address(), 4096);
	flash_area_erase_fast(ota_phy_offset, ota_phy_size);

	return 0;
}
