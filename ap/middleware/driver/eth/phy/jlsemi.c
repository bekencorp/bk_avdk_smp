// SPDX-License-Identifier: Apache-2.0
/*
 * JLSemi JL11x1 10/100M Ethernet PHY driver.
 *
 * This implementation is based on the public JL11x1 register interface. It
 * deliberately does not copy the firmware patch data or implementation from
 * JLSemi's GPL-licensed Linux driver.
 */

#include "miiphy.h"

#define JL11X1_PHY_ID                  0x937c4020
#define JL11X1_PHY_ID_MASK             0xfffffff0

#define JL11X1_PAGE_SELECT             31
#define JL11X1_PAGE_RMII               7
#define JL11X1_RMII_CTRL_REG           16
#define JL11X1_RMII_CLK_50M_INPUT      BIT(12)
#define JL11X1_RMII_TX_SKEW_MASK       (0xf << 8)
#define JL11X1_RMII_RX_SKEW_MASK       (0xf << 4)
#define JL11X1_RMII_MODE               BIT(3)
#define JL11X1_RMII_CRS_DV             BIT(2)

static int jl11x1_read_paged(struct phy_device *phydev, u16 page, u16 reg)
{
	int old_page;
	int value;
	int ret;

	old_page = phy_read(phydev, MDIO_DEVAD_NONE, JL11X1_PAGE_SELECT);
	if (old_page < 0)
		return old_page;

	ret = phy_write(phydev, MDIO_DEVAD_NONE, JL11X1_PAGE_SELECT, page);
	if (ret < 0)
		return ret;

	value = phy_read(phydev, MDIO_DEVAD_NONE, reg);

	ret = phy_write(phydev, MDIO_DEVAD_NONE, JL11X1_PAGE_SELECT,
			(u16)old_page);
	if (value < 0)
		return value;

	return ret < 0 ? ret : value;
}

static int jl11x1_modify_paged(struct phy_device *phydev, u16 page, u16 reg,
			       u16 mask, u16 set)
{
	int old_page;
	int value;
	int op_ret = 0;
	int ret;

	old_page = phy_read(phydev, MDIO_DEVAD_NONE, JL11X1_PAGE_SELECT);
	if (old_page < 0)
		return old_page;

	ret = phy_write(phydev, MDIO_DEVAD_NONE, JL11X1_PAGE_SELECT, page);
	if (ret < 0)
		return ret;

	value = phy_read(phydev, MDIO_DEVAD_NONE, reg);
	if (value >= 0) {
		u16 new_value = ((u16)value & ~mask) | set;

		if (new_value != (u16)value) {
			op_ret = phy_write(phydev, MDIO_DEVAD_NONE, reg,
					   new_value);
		}
	} else {
		op_ret = value;
	}

	ret = phy_write(phydev, MDIO_DEVAD_NONE, JL11X1_PAGE_SELECT,
			(u16)old_page);

	return op_ret < 0 ? op_ret : ret;
}

static int jl11x1_config(struct phy_device *phydev)
{
	int rmii_ctrl;
	int ret;

	if (phydev->interface != PHY_INTERFACE_MODE_RMII) {
		BK_LOGD(NULL, "JL11x1: unsupported interface mode %d\n",
			phydev->interface);
		return -1;
	}

	/* The PHY has to source REF_CLK: nothing on the board feeds it a 50MHz
	 * input, so leaving bit12 set starves the MAC of its RMII clock and the
	 * MAC soft reset never completes. */
	ret = jl11x1_modify_paged(phydev, JL11X1_PAGE_RMII,
				  JL11X1_RMII_CTRL_REG,
				  JL11X1_RMII_CLK_50M_INPUT, 0);
	if (ret < 0) {
		BK_LOGD(NULL, "JL11x1: failed to set REF_CLK output mode: %d\n",
			ret);
		return ret;
	}

	rmii_ctrl = jl11x1_read_paged(phydev, JL11X1_PAGE_RMII,
				     JL11X1_RMII_CTRL_REG);
	if (rmii_ctrl < 0) {
		BK_LOGD(NULL, "JL11x1: failed to read RMII control: %d\n",
			rmii_ctrl);
		return rmii_ctrl;
	}

	BK_LOGI(NULL,
		"JL11x1: id=0x%08x rmii=0x%04x mode=%s refclk=%s tx_skew=%u rx_skew=%u crs_dv=%u\n",
		phydev->phy_id, rmii_ctrl,
		(rmii_ctrl & JL11X1_RMII_MODE) ? "RMII" : "MII",
		(rmii_ctrl & JL11X1_RMII_CLK_50M_INPUT) ? "input" : "output",
		(rmii_ctrl & JL11X1_RMII_TX_SKEW_MASK) >> 8,
		(rmii_ctrl & JL11X1_RMII_RX_SKEW_MASK) >> 4,
		!!(rmii_ctrl & JL11X1_RMII_CRS_DV));

	if (!(rmii_ctrl & JL11X1_RMII_MODE)) {
		BK_LOGD(NULL, "JL11x1: hardware strap did not select RMII\n");
		return -1;
	}
	if (rmii_ctrl & JL11X1_RMII_CLK_50M_INPUT) {
		BK_LOGD(NULL, "JL11x1: failed to select REF_CLK output mode\n");
		return -1;
	}

	return genphy_config_aneg(phydev);
}

static struct phy_driver jl11x1_driver = {
	.name = "JLSemi JL11x1",
	.uid = JL11X1_PHY_ID,
	.mask = JL11X1_PHY_ID_MASK,
	.features = PHY_BASIC_FEATURES,
	.config = jl11x1_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
};

int phy_jlsemi_init(void)
{
	return phy_register(&jl11x1_driver);
}
