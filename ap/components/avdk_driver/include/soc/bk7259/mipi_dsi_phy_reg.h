#ifndef _MIPI_DSI_PHY_REG_H_
#define _MIPI_DSI_PHY_REG_H_

#if CONFIG_DSI_DRIVER

#define BK7259_NN_BASE_ADDR     0x4c240000

//====[NANNENG PHY]============================================================================================
#define reg_NN_PHY_R00             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x00)))
#define reg_NN_PHY_R04             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x04)))
#define reg_NN_PHY_R08             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x08)))
#define reg_NN_PHY_R0c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x0c)))
#define reg_NN_PHY_R10             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x10)))
#define reg_NN_PHY_R14             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x14)))
#define reg_NN_PHY_R18             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x18)))
#define reg_NN_PHY_R1c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x1c)))
#define reg_NN_PHY_R20             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x20)))
#define reg_NN_PHY_R24             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x24)))
#define reg_NN_PHY_R28             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x28)))
#define reg_NN_PHY_R2c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x2c)))
#define reg_NN_PHY_R30             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x30)))
#define reg_NN_PHY_R34             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x34)))
#define reg_NN_PHY_R38             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x38)))
#define reg_NN_PHY_R3c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x3c)))
#define reg_NN_PHY_R40             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x40)))
#define reg_NN_PHY_R44             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x44)))
#define reg_NN_PHY_R48             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x48)))
#define reg_NN_PHY_R4c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x4c)))
#define reg_NN_PHY_R50             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x50)))
#define reg_NN_PHY_R54             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x54)))
#define reg_NN_PHY_R58             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x58)))
#define reg_NN_PHY_R5c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x5c)))
#define reg_NN_PHY_R60             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x60)))
#define reg_NN_PHY_R64             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x64)))
#define reg_NN_PHY_R68             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x68)))
#define reg_NN_PHY_R6c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x6c)))
#define reg_NN_PHY_R70             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x70)))
#define reg_NN_PHY_R74             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x74)))
#define reg_NN_PHY_R78             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x78)))
#define reg_NN_PHY_R7c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x7c)))
#define reg_NN_PHY_R80             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x80)))
#define reg_NN_PHY_R84             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x84)))
#define reg_NN_PHY_R88             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x88)))
#define reg_NN_PHY_R8c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x8c)))
#define reg_NN_PHY_R90             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x90)))
#define reg_NN_PHY_R94             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x94)))
#define reg_NN_PHY_R98             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x98)))
#define reg_NN_PHY_R9c             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0x9c)))
#define reg_NN_PHY_Ra0             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0xa0)))
#define reg_NN_PHY_Ra4             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0xa4)))
#define reg_NN_PHY_Ra8             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0xa8)))
#define reg_NN_PHY_Rac             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0xac)))
#define reg_NN_PHY_Rb0             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0xb0)))
#define reg_NN_PHY_Rb4             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0xb4)))
#define reg_NN_PHY_Rb8             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0xb8)))
#define reg_NN_PHY_Rbc             (*((volatile unsigned int*) (BK7259_NN_BASE_ADDR + 0xbc)))

#define BK7259_EXDPHY_BASE_ADDR 0x4c230000

//====[SYNOPSYS PHY]============================================================================================
#define reg_EXDPHY_REG0            (*((volatile unsigned int*)(BK7259_EXDPHY_BASE_ADDR + 0x0*4)))
#define reg_EXDPHY_MN_RD           (*((volatile unsigned int*)(BK7259_EXDPHY_BASE_ADDR + 0x1*4)))

#endif
#endif
