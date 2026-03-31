/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/

/* Register interface based on the document version 1.1.1 */
    UFBCREG(HWIF_UFBC_HW_ID                         , 0  ,0xffff0000,       16,        0,RO,"ID of HW")
    UFBCREG(HWIF_UFBC_HW_VERSION                    , 0  ,0x000000ff,        0,        0,RO,"Version of HW (1.0.0).  [15:12]-Major [11:8]-Minor [7:0]-Build")
    UFBCREG(HWIF_UFBC_HW_BUILDDATE                  , 4  ,0xffffffff,        0,        0,RO,"HW package generation date in BCD code.For example 0x20230109")
    UFBCREG(HWIF_UFBC_BLOCK_TYPE                    , 8  ,0x00000008,        3,        0,RW,"0 meams 32x8 pixels and 1 means 16x16 pixels")
    UFBCREG(HWIF_UFBC_SPLIT                         , 8  ,0x00000004,        2,        0,RW,"enables block split mode")
    UFBCREG(HWIF_UFBC_YUV_TRANS                     , 8  ,0x00000002,        1,        0,RW,"enables the internal YUV transform")
    UFBCREG(HWIF_UFBC_ENABLE                        , 8  ,0x00000001,        0,        0,RW,"enable signal")
    UFBCREG(HWIF_UFBC_HDR_BASE_ADDR_LSB             , 12 ,0xffffffff,        0,        0,RW,"Header buffer low 32 bits address")
    UFBCREG(HWIF_UFBC_HDR_BASE_ADDR_MSB             , 16 ,0xffffffff,        0,        0,RW,"Header buffer high 32 bits address")
    UFBCREG(HWIF_UFBC_HDR_ADDR_LSB                  , 20 ,0xffffffff,        0,        0,RW,"Header start low 32 bits address")
    UFBCREG(HWIF_UFBC_HDR_ADDR_MSB                  , 24 ,0xffffffff,        0,        0,RW,"Header start high 32 bits address")
    UFBCREG(HWIF_UFBC_HDR_STRIDE                    , 28 ,0x0000ffff,        0,        0,RW,"Header stride for one superblock row unit 16 bytes")
    UFBCREG(HWIF_UFBC_CFG_ERR                       , 32 ,0x00000002,        1,        0,RW,"Config error interrupt")
    UFBCREG(HWIF_UFBC_DEC_ERR                       , 32 ,0x00000001,        0,        0,RW,"Decode error interrupt")
    UFBCREG(HWIF_UFBC_CFG_ERR_TYPE                  , 36 ,0x00020000,       17,        0,RW,"Config error mask")
    UFBCREG(HWIF_UFBC_DEC_ERR_TYPE                  , 36 ,0x00010000,       16,        0,RW,"Decoder error mask")
    UFBCREG(HWIF_UFBC_CFG_ERR_ENABLE                , 36 ,0x00000002,        1,        0,RW,"config error interrupt enable")
    UFBCREG(HWIF_UFBC_DEC_ERR_ENABLE                , 36 ,0x00000001,        0,        0,RW,"decode error interrupt enable")
