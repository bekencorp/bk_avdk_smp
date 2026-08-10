// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#include <stdint.h>
#include <soc/soc.h>
#include "partitions.h"

#define BL2_DS_RETENTION_MAGIC      (0x46584252u) /* Little-endian "RBXF": Retention Boot XIP Flash record. */
#define BL2_DS_RETENTION_WORDS      (8u)
#define BL2_DS_EXPECTED_BEGIN       CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET
#define BL2_DS_EXPECTED_END         (CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET +  CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE)
#define BL2_DS_EXPECTED_REMAP       (CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET - CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET)

typedef struct {
	uint32_t magic;
	uint32_t magic_inv;
	uint32_t reg16;
	uint32_t reg17;
	uint32_t reg18;
	uint8_t reg19;
	uint8_t reg0b_bit25;
	uint8_t reserved[2];
	uint32_t xor_check;
	uint32_t sum_check;
} bl2_ds_flash_retention_t;

_Static_assert(sizeof(bl2_ds_flash_retention_t) ==
	       BL2_DS_RETENTION_WORDS * sizeof(uint32_t),
	       "fastboot retention record must be 32 bytes");

/*
 * The secure linker places this NOLOAD record at the fixed BL2/TF-M ABI address
 * immediately after shared data. Write magic last so a reset during the update
 * cannot commit a partially written record.
 */
static volatile bl2_ds_flash_retention_t s_bl2_ds_flash_retention __attribute__((section(".bl2_fastboot_retention"), aligned(4), used));

void tfm_deepsleep_fastboot_save_xip(void)
{
	volatile bl2_ds_flash_retention_t *record = &s_bl2_ds_flash_retention;
	uint32_t reg16 = REG_READ(SOC_FLASH_REG_BASE + 0x16u * 4u);
	uint32_t reg17 = REG_READ(SOC_FLASH_REG_BASE + 0x17u * 4u);
	uint32_t reg18 = REG_READ(SOC_FLASH_REG_BASE + 0x18u * 4u);
	uint32_t reg19 = REG_READ(SOC_FLASH_REG_BASE + 0x19u * 4u) & 0x1u;
	uint32_t reg0b_bit25 = (REG_READ(SOC_FLASH_REG_BASE + 0x0bu * 4u) >> 25) & 0x1u;

	record->magic = 0u;
	__asm volatile("dsb" ::: "memory");

	/* Do not commit a retention record from an unexpected XIP layout. */
	if (reg16 != BL2_DS_EXPECTED_BEGIN ||
	    reg17 != BL2_DS_EXPECTED_END ||
	    reg18 != BL2_DS_EXPECTED_REMAP ||
	    reg19 > 1u ||
	    reg0b_bit25 != 1u) {
		return;
	}

	record->magic_inv = ~BL2_DS_RETENTION_MAGIC;
	record->reg16 = reg16;
	record->reg17 = reg17;
	record->reg18 = reg18;
	record->reg19 = reg19;
	record->reg0b_bit25 = reg0b_bit25;
	record->reserved[0] = 0u;
	record->reserved[1] = 0u;
	record->xor_check = BL2_DS_RETENTION_MAGIC ^ reg16 ^ reg17 ^ reg18 ^
			    reg19 ^ reg0b_bit25 ^
			    record->reserved[0] ^ record->reserved[1];
	record->sum_check = BL2_DS_RETENTION_MAGIC + reg16 + reg17 + reg18 +
			    reg19 + reg0b_bit25 +
			    record->reserved[0] + record->reserved[1];

	__asm volatile("dsb" ::: "memory");
	record->magic = BL2_DS_RETENTION_MAGIC;
	__asm volatile("dsb" ::: "memory");
}
