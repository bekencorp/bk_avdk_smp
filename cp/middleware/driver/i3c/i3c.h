/**
 * @file i3c.h
 * @brief I3C umbrella: IP register macros (addI3C_* in i3c_common.h) + common/master/slave APIs.
 *        Include this when building under I3C/ to get addI3C_* and Beken compat macro.
 */
#ifndef I3C_I3C_H
#define I3C_I3C_H

#include "i3c_common.h"
#include "i3c_master.h"
#include "i3c_slave.h"

/* Compatibility: test code reads Beken state via addI3C_BEKEN_Reg0x2 */
#define addI3C_BEKEN_Reg0x2  I3C_BEKEN_REG(2)

#endif /* I3C_I3C_H */
