#include <gtest/gtest.h>

extern "C" {
#include "sys_sw_regs.h"
}

/* -------------------------------------------------------------------------
 * Helper: reset all fields to 0 before each test by writing zero values.
 * ------------------------------------------------------------------------- */
class SysSwRegsTest : public ::testing::Test {
protected:
    void SetUp() override {
        bk_sys_sw_regs_set_psram_power_down(0);
        bk_sys_sw_regs_set_cp_reset_reason(0);
        bk_sys_sw_regs_set_ap_reset_reason(0);
    }
};

/* -------------------------------------------------------------------------
 * Initial value tests
 * ------------------------------------------------------------------------- */

TEST_F(SysSwRegsTest, InitialPsramPowerDownIsZero)
{
    ASSERT_EQ(0u, bk_sys_sw_regs_get_psram_power_down());
}

TEST_F(SysSwRegsTest, InitialCpResetReasonIsZero)
{
    ASSERT_EQ(0u, bk_sys_sw_regs_get_cp_reset_reason());
}

TEST_F(SysSwRegsTest, InitialApResetReasonIsZero)
{
    ASSERT_EQ(0u, bk_sys_sw_regs_get_ap_reset_reason());
}

/* -------------------------------------------------------------------------
 * Read/write tests
 * ------------------------------------------------------------------------- */

TEST_F(SysSwRegsTest, SetGetPsramPowerDown)
{
    bk_sys_sw_regs_set_psram_power_down(0x123u);
    ASSERT_EQ(0x123u, bk_sys_sw_regs_get_psram_power_down());
}

TEST_F(SysSwRegsTest, SetGetCpResetReason)
{
    bk_sys_sw_regs_set_cp_reset_reason(0xABCDu);
    ASSERT_EQ(0xABCDu, bk_sys_sw_regs_get_cp_reset_reason());
}

TEST_F(SysSwRegsTest, SetGetApResetReason)
{
    bk_sys_sw_regs_set_ap_reset_reason(0xDEADBEEFu);
    ASSERT_EQ(0xDEADBEEFu, bk_sys_sw_regs_get_ap_reset_reason());
}

/* -------------------------------------------------------------------------
 * Multiple write / overwrite tests
 * ------------------------------------------------------------------------- */

TEST_F(SysSwRegsTest, OverwritePsramPowerDown)
{
    bk_sys_sw_regs_set_psram_power_down(0x1u);
    bk_sys_sw_regs_set_psram_power_down(0x2u);
    bk_sys_sw_regs_set_psram_power_down(0x3u);
    ASSERT_EQ(0x3u, bk_sys_sw_regs_get_psram_power_down());
}

TEST_F(SysSwRegsTest, OverwriteCpResetReason)
{
    bk_sys_sw_regs_set_cp_reset_reason(0x10u);
    bk_sys_sw_regs_set_cp_reset_reason(0x20u);
    ASSERT_EQ(0x20u, bk_sys_sw_regs_get_cp_reset_reason());
}

TEST_F(SysSwRegsTest, OverwriteApResetReason)
{
    bk_sys_sw_regs_set_ap_reset_reason(0xFFFFFFFFu);
    bk_sys_sw_regs_set_ap_reset_reason(0x0u);
    ASSERT_EQ(0x0u, bk_sys_sw_regs_get_ap_reset_reason());
}

/* -------------------------------------------------------------------------
 * Independence tests: writing one field does not affect others
 * ------------------------------------------------------------------------- */

TEST_F(SysSwRegsTest, FieldsAreIndependent)
{
    bk_sys_sw_regs_set_psram_power_down(0x11u);
    bk_sys_sw_regs_set_cp_reset_reason(0x22u);
    bk_sys_sw_regs_set_ap_reset_reason(0x33u);

    ASSERT_EQ(0x11u, bk_sys_sw_regs_get_psram_power_down());
    ASSERT_EQ(0x22u, bk_sys_sw_regs_get_cp_reset_reason());
    ASSERT_EQ(0x33u, bk_sys_sw_regs_get_ap_reset_reason());
}
