// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: minimal clock-monitor (CKMN) start.
//
// The Beken-patched TF-M spm core/main.c calls bk_ckmn_start() to enable the
// 26M/32K clock-accuracy auto-switch. The full SDK ckmn_driver.c additionally
// registers an interrupt ISR and pulls in os/sys_driver dependencies that the
// secure world does not need: the auto-switch correction runs in hardware once
// the targets are programmed. This self-contained version drives the CKMN
// registers directly via the SDK inline ckmn_ll.h (pure register header).

#include <stdint.h>
#include <common/bk_include.h>
#include <driver/ckmn.h>
#include "ckmn_ll.h"

static ckmn_hw_t *const s_ckmn_hw = (ckmn_hw_t *)SOC_CKMN_REG_BASE;

void bk_ckmn_start(void)
{
	ckmn_ll_set_soft_reset(s_ckmn_hw);

	ckmn_ll_set_26m_target(s_ckmn_hw, CKMN_TARGET_4PER);
	ckmn_ll_set_32k_target(s_ckmn_hw, CKMN_TARGET_4PER);

	ckmn_ll_autosw_26m_enable(s_ckmn_hw);
	ckmn_ll_corr_26m_enable(s_ckmn_hw);
	ckmn_ll_autosw_32k_enable(s_ckmn_hw);
	ckmn_ll_corr_32k_enable(s_ckmn_hw);
}
