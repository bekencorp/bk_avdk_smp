/*------------------------------------------------------------------------------
--        Copyright (c) 2015, VeriSilicon Inc. All rights reserved            --

--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
*/

#ifndef _USER_FREERTOS_H_
#define _USER_FREERTOS_H_

#include "basetype.h"

#include <stddef.h>   // size_t
#include <stdint.h>
#include <string.h>

//u32, MSB 4bit for device type, LSB 28bit for reference count
enum {
  ENC_FD = 0x1,
  DEC_FD = 0x2,
  MEM_FD = 0x3
  //TODO for other device, can exist 15 kinds of evices
};

int Platform_init();
int h264d_freertos_open(const char* dev_name, int flag);
void h264d_freertos_close(int fd);
// long freertos_ioctl(int *filp, unsigned int cmd, unsigned long arg);
long h264d_freertos_ioctl(int filp, unsigned int cmd, void *arg);
int hx170dec_init(void);      //7259debug@zhilei
int hx170dec_open(void *inode, int *filp);
int hx170dec_release(void *inode, int *filp);
void hx170dec_cleanup(void);
long hx170dec_ioctl(int *filp, unsigned int cmd, unsigned long arg);
#endif /* _USER_FREERTOS_H_ */
