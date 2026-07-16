// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * Dhara NAND driver glue for the BK7259 QSPI SPI-NAND.
 *
 * Dhara defines a fixed set of dhara_nand_* functions that the integrator must
 * implement (see dhara/nand.h). Each maps a Dhara page/block (relative to the
 * FTL partition) onto a physical QSPI-NAND page/block and calls the SDK driver.
 *
 * Dhara stores all of its own metadata inside the main page area (checkpoint
 * pages), so only main-area page read/prog/erase plus bad-block services are
 * needed here; the on-die ECC of the ZB35Q01CYIG protects every read/prog.
 */

#include <common/bk_include.h>

#if CONFIG_DHARA_FTL

#include <driver/qspi_types.h>
#include <driver/qspi_flash.h>
#include <driver/hal/hal_qspi_types.h>
#include "nand_ftl_priv.h"

int dhara_nand_is_bad(const struct dhara_nand *n, dhara_block_t b)
{
	struct nand_ftl_ctx *c = nand_ftl_ctx_of(n);

	if (nand_ftl_bad_get(c, b)) {
		return 1;
	}

	bool is_bad = false;
	if (bk_qspi_flash_nand_is_factory_bad(c->id, nand_ftl_phys_block(c, b), &is_bad) != BK_OK) {
		/* If we cannot even read the marker, treat the block as bad. */
		nand_ftl_bad_set(c, b);
		return 1;
	}

	if (is_bad) {
		nand_ftl_bad_set(c, b);
	}
	return is_bad ? 1 : 0;
}

void dhara_nand_mark_bad(const struct dhara_nand *n, dhara_block_t b)
{
	struct nand_ftl_ctx *c = nand_ftl_ctx_of(n);

	nand_ftl_bad_set(c, b);
	/* Persist across reboot; best-effort (a failing block may reject writes). */
	(void)bk_qspi_flash_nand_mark_bad(c->id, nand_ftl_phys_block(c, b));
}

int dhara_nand_erase(const struct dhara_nand *n, dhara_block_t b, dhara_error_t *err)
{
	struct nand_ftl_ctx *c = nand_ftl_ctx_of(n);

	if (bk_qspi_flash_nand_block_erase(c->id, nand_ftl_phys_block(c, b)) != BK_OK) {
		dhara_set_error(err, DHARA_E_BAD_BLOCK);
		return -1;
	}
	return 0;
}

int dhara_nand_prog(const struct dhara_nand *n, dhara_page_t p,
		    const uint8_t *data, dhara_error_t *err)
{
	struct nand_ftl_ctx *c = nand_ftl_ctx_of(n);

	if (bk_qspi_flash_nand_page_program(c->id, nand_ftl_phys_page(c, p), 0,
					    data, NAND_FTL_PAGE_SIZE) != BK_OK) {
		dhara_set_error(err, DHARA_E_BAD_BLOCK);
		return -1;
	}
	return 0;
}

int dhara_nand_is_free(const struct dhara_nand *n, dhara_page_t p)
{
	struct nand_ftl_ctx *c = nand_ftl_ctx_of(n);
	bool erased = false;

	if (bk_qspi_flash_nand_page_is_erased(c->id, nand_ftl_phys_page(c, p), &erased) != BK_OK) {
		/* Ambiguous: report "not free" so the journal never mistakes a
		 * troubled page for the erased write frontier. */
		return 0;
	}
	return erased ? 1 : 0;
}

int dhara_nand_read(const struct dhara_nand *n, dhara_page_t p,
		    size_t offset, size_t length,
		    uint8_t *data, dhara_error_t *err)
{
	struct nand_ftl_ctx *c = nand_ftl_ctx_of(n);

	bk_err_t ret = bk_qspi_flash_nand_page_read(c->id, nand_ftl_phys_page(c, p),
						    (uint32_t)offset, data, (uint32_t)length);
	if (ret == BK_ERR_QSPI_NAND_ECC_FAIL) {
		dhara_set_error(err, DHARA_E_ECC);
		return -1;
	}
	if (ret != BK_OK) {
		dhara_set_error(err, DHARA_E_ECC);
		return -1;
	}
	return 0;
}

int dhara_nand_copy(const struct dhara_nand *n,
		    dhara_page_t src, dhara_page_t dst, dhara_error_t *err)
{
	struct nand_ftl_ctx *c = nand_ftl_ctx_of(n);

	bk_err_t ret = bk_qspi_flash_nand_page_read(c->id, nand_ftl_phys_page(c, src), 0,
						    c->copy_buf, NAND_FTL_PAGE_SIZE);
	if (ret == BK_ERR_QSPI_NAND_ECC_FAIL) {
		dhara_set_error(err, DHARA_E_ECC);
		return -1;
	}
	if (ret != BK_OK) {
		dhara_set_error(err, DHARA_E_ECC);
		return -1;
	}

	if (bk_qspi_flash_nand_page_program(c->id, nand_ftl_phys_page(c, dst), 0,
					    c->copy_buf, NAND_FTL_PAGE_SIZE) != BK_OK) {
		dhara_set_error(err, DHARA_E_BAD_BLOCK);
		return -1;
	}
	return 0;
}

#endif /* CONFIG_DHARA_FTL */
