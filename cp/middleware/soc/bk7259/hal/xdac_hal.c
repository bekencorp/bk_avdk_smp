// Copyright 2020-2021 Beken
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

#include "xdac_hal.h"
#include "xdac0_ll.h"
#include "xdac1_ll.h"

bk_err_t xdac_hal_init(uint8_t ch, xdac_hal_t *hal)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        hal->xdac0_hw = (xdac0_hw_t *)XDAC0_LL_REG_BASE;
    }
    else if(XDAC_1 == ch)
    {
        hal->xdac1_hw = (xdac1_hw_t *)XDAC1_LL_REG_BASE;
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;
}

uint32_t xdac_hal_get_fifo_empty_int(uint8_t ch)
{    
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg5_fifo_empty_int();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg5_fifo_empty_int();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

uint32_t xdac_hal_get_fifo_full_int(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg5_fifo_full_int();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg5_fifo_full_int();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

uint32_t xdac_hal_get_fifo_near_full_int(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg5_fifo_near_full_int();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg5_fifo_near_full_int();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

uint32_t xdac_hal_get_fifo_near_empty_int(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg5_fifo_near_empty_int();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg5_fifo_near_empty_int();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

uint32_t xdac_hal_get_fifo_empty_int_en(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg6_fifo_empty_int_en();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg6_fifo_empty_int_en();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

uint32_t xdac_hal_get_fifo_full_int_en(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg6_fifo_full_int_en();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg6_fifo_full_int_en();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

uint32_t xdac_hal_get_fifo_near_full_int_en(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg6_fifo_near_full_int_en();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg6_fifo_near_full_int_en();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

uint32_t xdac_hal_get_fifo_near_empty_int_en(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg6_fifo_near_empty_int_en();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg6_fifo_near_empty_int_en();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}


bk_err_t xdac_hal_set_fifo_empty_int_en(uint8_t ch, uint8_t en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg6_fifo_empty_int_en(en);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg6_fifo_empty_int_en(en);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;
}

bk_err_t xdac_hal_set_fifo_full_int_en(uint8_t ch, uint8_t en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg6_fifo_full_int_en(en);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg6_fifo_full_int_en(en);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;
}

bk_err_t xdac_hal_set_fifo_near_full_int_en(uint8_t ch, uint8_t en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg6_fifo_near_full_int_en(en);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg6_fifo_near_full_int_en(en);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;
}

bk_err_t xdac_hal_set_fifo_near_empty_int_en(uint8_t ch, uint8_t en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg6_fifo_near_empty_int_en(en);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg6_fifo_near_empty_int_en(en);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;
}

bk_err_t xdac_hal_set_soft_reset(uint8_t ch, uint8_t rst)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg2_soft_reset(rst);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg2_soft_reset(rst);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

bk_err_t xdac_hal_set_dac_clk_div(uint8_t ch, uint16_t clk_div)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg4_dac_clk_div(clk_div);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg4_dac_clk_div(clk_div);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

uint32_t xdac_hal_get_dac_clk_div(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg4_dac_clk_div();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg4_dac_clk_div();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

bk_err_t xdac_hal_set_dac_clk_en(uint8_t ch, uint8_t dac_clk_en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg4_dac_clk_en(dac_clk_en);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg4_dac_clk_en(dac_clk_en);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

bk_err_t xdac_hal_set_fifo_enable(uint8_t ch, uint8_t fifo_en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg4_fifo_enable(fifo_en);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg4_fifo_enable(fifo_en);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

bk_err_t xdac_hal_set_dac_rthrd(uint8_t ch, uint8_t dac_rthrd)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg7_dac_rthrd(dac_rthrd);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg7_dac_rthrd(dac_rthrd);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

uint32_t xdac_hal_get_dac_rthrd(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg7_dac_rthrd();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg7_dac_rthrd();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

bk_err_t xdac_hal_set_dac_wthrd(uint8_t ch, uint8_t dac_wthrd)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg7_dac_wthrd(dac_wthrd);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg7_dac_wthrd(dac_wthrd);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

uint32_t xdac_hal_get_dac_wthrd(uint8_t ch)
{
    if(XDAC_0 == ch)
    {
        return xdac0_ll_get_reg7_dac_wthrd();
    }
    else if(XDAC_1 == ch)
    {
        return xdac1_ll_get_reg7_dac_wthrd();
    }
    else
    {
        return BK_ERR_XDAC_CH_INVALID;
    }
}

bk_err_t xdac_hal_set_dac_enable(uint8_t ch, uint8_t dac_en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        xdac0_ll_set_reg4_dac_enable(dac_en);
    }
    else if(XDAC_1 == ch)
    {
        xdac1_ll_set_reg4_dac_enable(dac_en);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

uint32_t xdac_hal_get_fifo_addr(uint8_t ch)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        return XDAC0_REG8_ADDR;
    }
    else if(XDAC_1 == ch)
    {
        return XDAC1_REG8_ADDR;
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

bk_err_t xdac_hal_set_int_en(uint8_t ch, uint8_t int_type, uint8_t en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        switch(int_type)
        {
            case XDAC_EMPTY_INT:
            {
                xdac0_ll_set_reg6_fifo_empty_int_en(en);
                break;
            }
            case XDAC_FULL_INT:
            {
                xdac0_ll_set_reg6_fifo_full_int_en(en);
                break;
            }
            case XDAC_NEAR_FULL_INT:
            {
                xdac0_ll_set_reg6_fifo_near_full_int_en(en);
                break;
            }
            case XDAC_NEAR_EMPTY_INT:
            {
                xdac0_ll_set_reg6_fifo_near_empty_int_en(en);
                break;
            }
            default:
            {
                ret = BK_ERR_XDAC_INT_TYPE_IS_INVALID;
                break;
            }
        }
    }
    else if(XDAC_1 == ch)
    {
        switch(int_type)
        {
            case XDAC_EMPTY_INT:
            {
                xdac1_ll_set_reg6_fifo_empty_int_en(en);
                break;
            }
            case XDAC_FULL_INT:
            {
                xdac1_ll_set_reg6_fifo_full_int_en(en);
                break;
            }
            case XDAC_NEAR_FULL_INT:
            {
                xdac1_ll_set_reg6_fifo_near_full_int_en(en);
                break;
            }
            case XDAC_NEAR_EMPTY_INT:
            {
                xdac1_ll_set_reg6_fifo_near_empty_int_en(en);
                break;
            }
            default:
            {
                ret = BK_ERR_XDAC_INT_TYPE_IS_INVALID;
                break;
            }
        }
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;
}

