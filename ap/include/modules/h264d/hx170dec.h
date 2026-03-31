/*
 * Copyright (c) 2015, Verisilicon Inc. - All Rights Reserved
 * Copyright (c) 2011-2014, Google Inc. - All Rights Reserved
 *
 * This software is dual licensed, either GPL-2.0 or BSD-3-Clause, at your
 * option.
 *
 ********************************************************************************
 *
 * GPL-2.0
 *
 ********************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 ********************************************************************************
 *
 * Alternatively, This software may be distributed under the terms of
 * BSD-3-Clause, in which case the following provisions apply instead of the ones
 * mentioned above :
 *
 ********************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ********************************************************************************
 */

#ifndef _HX170DEC_H_
#define _HX170DEC_H_

// #ifdef __FREERTOS__
// #include "dev_common_freertos.h"
// #else
// #include <linux/ioctl.h>
// #include <linux/types.h>
// #endif
#include "dev_common_freertos.h"	//7259debug@zhilei

#undef PDEBUG
#ifdef MEMALLOC_DEBUG
#ifdef __KERNEL__
#define PDEBUG(fmt, args...)  pr_info("memalloc: " fmt, ##args)
#else
#define PDEBUG(fmt, args...)  printf(fmt, ## args)//fprintf(stderr, fmt, ##args)
#endif
#else
#define PDEBUG(fmt, args...)
#endif

struct core_desc {
	__u32 id; /* id of the core */
	__u32 *regs; /* pointer to user registers */
	__u32 size; /* size of register space */
};

/* Use 'k' as magic number */
#define HX170DEC_IOC_MAGIC 'k'

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

#define HX170DEC_PP_INSTANCE  _IO(HX170DEC_IOC_MAGIC, 1)
#define HX170DEC_HW_PERFORMANCE  _IO(HX170DEC_IOC_MAGIC, 2)
#define HX170DEC_IOCGHWOFFSET  _IOR(HX170DEC_IOC_MAGIC, 3, unsigned long *)
#define HX170DEC_IOCGHWIOSIZE  _IOR(HX170DEC_IOC_MAGIC, 4, unsigned int *)

#define HX170DEC_IOC_CLI  _IO(HX170DEC_IOC_MAGIC, 5)
#define HX170DEC_IOC_STI  _IO(HX170DEC_IOC_MAGIC, 6)
#define HX170DEC_IOC_MC_OFFSETS  _IOR(HX170DEC_IOC_MAGIC, 7, unsigned long *)
#define HX170DEC_IOC_MC_CORES  _IOR(HX170DEC_IOC_MAGIC, 8, unsigned int *)

#define HX170DEC_IOCS_DEC_PUSH_REG                                             \
	_IOW(HX170DEC_IOC_MAGIC, 9, struct core_desc *)
#define HX170DEC_IOCS_PP_PUSH_REG                                              \
	_IOW(HX170DEC_IOC_MAGIC, 10, struct core_desc *)

#define HX170DEC_IOCH_DEC_RESERVE  _IO(HX170DEC_IOC_MAGIC, 11)
#define HX170DEC_IOCT_DEC_RELEASE  _IO(HX170DEC_IOC_MAGIC, 12)
#define HX170DEC_IOCQ_PP_RESERVE  _IO(HX170DEC_IOC_MAGIC, 13)
#define HX170DEC_IOCT_PP_RELEASE  _IO(HX170DEC_IOC_MAGIC, 14)

#define HX170DEC_IOCX_DEC_WAIT _IOWR(HX170DEC_IOC_MAGIC, 15, struct core_desc *)
#define HX170DEC_IOCX_PP_WAIT  _IOWR(HX170DEC_IOC_MAGIC, 16, struct core_desc *)

#define HX170DEC_IOCS_DEC_PULL_REG                                             \
	_IOWR(HX170DEC_IOC_MAGIC, 17, struct core_desc *)
#define HX170DEC_IOCS_PP_PULL_REG                                              \
	_IOWR(HX170DEC_IOC_MAGIC, 18, struct core_desc *)

#define HX170DEC_IOCG_CORE_WAIT  _IOR(HX170DEC_IOC_MAGIC, 19, int *)

#define HX170DEC_IOX_ASIC_ID  _IOWR(HX170DEC_IOC_MAGIC, 20, __u32 *)

#define HX170DEC_DEBUG_STATUS  _IO(HX170DEC_IOC_MAGIC, 29)

#define HX170DEC_IOC_MAXNR  29


// int __init hx170dec_init(void);	//7259debug@zhilei
// long hx170dec_ioctl(int filp, unsigned int cmd, unsigned long arg);//7259debug@zhilei
// long hx170dec_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);//7259debug@zhilei

#endif /* !_HX170DEC_H_ */
