/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : H2 Encoder Wrapper Layer for OS services
--
------------------------------------------------------------------------------*/

#ifndef __EWL_H__
#define __EWL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"
#include "vsi_queue.h"
#include "osal.h"
// #include "encswhwregisters.h"
#include "vsi_string.h"

#include "ewl_hwcfg.h"


/**
 * \defgroup ewl_info Core Information
 *
 * @{
 */

/** \brief The total register number of an encoder core. */
#define ASIC_SWREG_AMOUNT 512

/* HW ID check. */
/** \brief The bit mask to obtain the product ID from the hardware ID. */
#define HW_ID_PRODUCT_MASK 0xFFFF0000
/** \brief The bit mask to obtain the major version number from the hardware ID. */
#define HW_ID_MAJOR_NUMBER_MASK 0x0000FF00
/** \brief The bit mask to obtain the minor version number from the hardware ID. */
#define HW_ID_MINOR_NUMBER_MASK 0x000000FF
/** \brief The product ID derived from the hardware ID. */
#define HW_ID_PRODUCT(x) (((x & HW_ID_PRODUCT_MASK) >> 16))
/** \brief The major version number derived from the hardware ID. */
#define HW_ID_MAJOR_NUMBER(x) (((x & HW_ID_MAJOR_NUMBER_MASK) >> 8))
/** \brief The minor version number derived from the hardware ID. */
#define HW_ID_MINOR_NUMBER(x) ((x & HW_ID_MINOR_NUMBER_MASK))

/** \brief The hardware engine is H2, which is the first generation video encoder. */
#define HW_ID_PRODUCT_H2 0x4832
/** \brief The hardware engine is VC8000E.*/
#define HW_ID_PRODUCT_VC8000E 0x8000
/** \brief The hardware engine is VC9000E.*/
#define HW_ID_PRODUCT_VC9000E 0x9000
/** \brief The hardware engine is VC9000LE. */
#define HW_ID_PRODUCT_VC9000LE 0x9010
/** \brief The hardware engine is VC9800E. */
#define HW_ID_PRODUCT_VC9800E 0x9800

/** \brief Specifies whether the product ID is H2 series. */
#define HW_PRODUCT_H2(x) (HW_ID_PRODUCT_H2 == HW_ID_PRODUCT(x))
/** \brief Specifies whether the product ID is VC8000E. */
#define HW_PRODUCT_VC8000E(x) (HW_ID_PRODUCT_VC8000E == HW_ID_PRODUCT(x))
//0x80006000
/** \brief Specifies whether the product ID is VC8000E version 6.0. For details, see Section <i>Macro Definition</i>.*/
#define HW_PRODUCT_SYSTEM60(x)                    \
  ((HW_ID_PRODUCT_VC8000E == HW_ID_PRODUCT(x)) && \
   (HW_ID_MAJOR_NUMBER(x) == 0x60))
//0x80006010
/** \brief Specifies whether the product ID is VC8000E version 6.1. */
#define HW_PRODUCT_SYSTEM6010(x) \
  (HW_PRODUCT_SYSTEM60(x) && (HW_ID_MINOR_NUMBER(x) == 0x10))
//0x90001000
/** \brief Specifies whether the product ID is VC9000E. */
#define HW_PRODUCT_VC9000(x)        \
  (HW_ID_PRODUCT_VC9000E == HW_ID_PRODUCT(x) || HW_ID_PRODUCT_VC9800E == HW_ID_PRODUCT(x))
//0x90101000
/** \brief Specifies whether the product ID is VC8000LE. */
#define HW_PRODUCT_VC9000LE(x) (HW_ID_PRODUCT_VC9000LE == HW_ID_PRODUCT(x))

/** \brief The ASIC_ID, which indicates when the hardware can support BUILD_ID. */
#define MIN_ASIC_ID_WITH_BUILD_ID 0x80009100

/** \brief The register offset of the build ID register. */
#define HWIF_REG_BUILD_ID (509)
/** \brief The register offset of the build revision register. */
#define HWIF_REG_BUILD_REV (510)
/** \brief The register offset of the build date register. */
#define HWIF_REG_BUILD_DATE (511)

/** \brief  The hardware register configuration_1. */
#define HWIF_REG_CFG1 (80)
/** \brief The hardware register configuration_2. */
#define HWIF_REG_CFG2 (214)
/** \brief  The hardware register configuration_3.*/
#define HWIF_REG_CFG3 (226)
/** \brief The hardware register configuration_4. */
#define HWIF_REG_CFG4 (287)
/** \brief The hardware register configuration_AXI. */
#define HWIF_REG_CFGAXI (319)
/** \brief The hardware register configuration_5. */
#define HWIF_REG_CFG5 (430)
/** \brief The number of hardware configuration registers. */
#define HWIF_CFG_NUM 7

/** \brief The bit position that specifies core mode in the core information words. */
#define CORE_INFO_MODE_OFFSET 31
/** \brief The start bit position that specifies the core amount in the core information words. */
#define CORE_INFO_AMOUNT_OFFSET 28
/** \brief The maximum number of encoder cores that the software can support. */
#define MAX_SUPPORT_CORE_NUM 2//4//FIXME@RAMOPT

/* HW status register bits */
/** \brief The bit mask for the stream segment ready flag. */
#define ASIC_STATUS_SEGMENT_READY 0x1000
/** \brief The bit mask for the fuse error flag. */
#define ASIC_STATUS_FUSE_ERROR 0x200
/** \brief The bit mask for the slice ready flag. */
#define ASIC_STATUS_SLICE_READY 0x100
/** \brief The bit mask for the flag, which indicates whether a line buffer is used. */
#define ASIC_STATUS_LINE_BUFFER_DONE 0x080

/** \brief The bit mask for the flag, which indicates whether the slice information is updated in time. */
#define ASIC_STATUS_POLL_SLICEINFO_TIMEOUT 0x2000

/** \brief The bit mask for the external frame buffer compressor (UFBC) status flag, which
 * indicates whether an decode error occurs when UFBC is used to fetch the input picture data. */
#define ASIC_STATUS_UFBC_DEC_ERR 0x4000  //bit14
/** \brief The bit mask for the external frame buffer compressor (UFBC) status flag, which
 * indicates whether an config error occurs when UFBC is used to fetch the input picture data. */
#define ASIC_STATUS_UFBC_CFG_ERR 0x8000  //bit15

/** \brief The bit mask for the hardware timeout flag. */
#define ASIC_STATUS_HW_TIMEOUT 0x040

/** \brief The bit mask for the flag, which indicates the stream buffer is full. */
#define ASIC_STATUS_BUFF_FULL 0x020
/** \brief The bit mask for the flag, which indicates the hardware reset is completed. */
#define ASIC_STATUS_HW_RESET 0x010
/** \brief The bit mask for the bus error flag. */
#define ASIC_STATUS_ERROR 0x008
/** \brief The bit mask for the flag, which indicates whether the encoding of one frame is completed. */
#define ASIC_STATUS_FRAME_READY 0x004
/** \brief The bit mask for the flag, which indicates whether IRQ occurs. */
#define ASIC_IRQ_LINE 0x001

/** \brief The bit mask for valid status flags. For details, see Section <i>Macro Definition</i>. */
#define ASIC_STATUS_ALL                                                    \
  (ASIC_STATUS_SEGMENT_READY | ASIC_STATUS_FUSE_ERROR                     \
   | ASIC_STATUS_SLICE_READY | ASIC_STATUS_LINE_BUFFER_DONE                  \
   | ASIC_STATUS_HW_TIMEOUT | ASIC_STATUS_BUFF_FULL | ASIC_STATUS_HW_RESET  \
   | ASIC_STATUS_ERROR | ASIC_STATUS_FRAME_READY | ASIC_STATUS_UFBC_DEC_ERR \
   | ASIC_STATUS_UFBC_CFG_ERR | ASIC_STATUS_POLL_SLICEINFO_TIMEOUT)

/* Return values */
/** \brief API runs successfully. */
#define EWL_OK 0
/** \brief Errors occur when executing the function, such as the specified feature is not supported. */
#define EWL_ERROR -1
/** \brief The function execution fails, because the specified feature is not supported. */
#define EWL_NOT_SUPPORT -2

/** \brief The hardware is enabled properly even if an error is detected. */
#define EWL_HW_WAIT_OK EWL_OK
/** \brief An error is detected by the software. */
#define EWL_HW_WAIT_ERROR EWL_ERROR
/** \brief The hardware times out. */
#define EWL_HW_WAIT_TIMEOUT 1

/* Hardware configuration values */
/** \brief The bus type is unset. */
#define EWL_HW_BUS_TYPE_UNKNOWN 0
/** \brief The bus type is AHB. */
#define EWL_HW_BUS_TYPE_AHB 1
/** \brief The bus type is OCP.*/
#define EWL_HW_BUS_TYPE_OCP 2
/** \brief The bus type is AXI. */
#define EWL_HW_BUS_TYPE_AXI 3
/** \brief The bus type is PCI. */
#define EWL_HW_BUS_TYPE_PCI 4

/** \brief The bus width is unset. */
#define EWL_HW_BUS_WIDTH_UNKNOWN 0
/** \brief The bus width is 32 bits.*/
#define EWL_HW_BUS_WIDTH_32BITS 1
/** \brief The bus width is 64 bits. */
#define EWL_HW_BUS_WIDTH_64BITS 2
/** \brief The bus width is 128 bits. */
#define EWL_HW_BUS_WIDTH_128BITS 3

/** \brief The synthesis language is unset. */
#define EWL_HW_SYNTHESIS_LANGUAGE_UNKNOWN 0
/** \brief The synthesis language is VHDL. */
#define EWL_HW_SYNTHESIS_LANGUAGE_VHDL 1
/** \brief The synthesis language is VeriLog. */
#define EWL_HW_SYNTHESIS_LANGUAGE_VERILOG 2

/** \brief The current hardware configuration does not support the specified feature. */
#define EWL_HW_CONFIG_NOT_SUPPORTED 0
/** \brief The current hardware configuration supports the specified feature. */
#define EWL_HW_CONFIG_ENABLED 1

#ifndef VCECFG_MAX_CORE_NUM
/** \brief The supported maximum number of cores. */
#define EWL_MAX_CORES 4
#else
/** \brief The supported maximum number of cores. */
#define EWL_MAX_CORES VCECFG_MAX_CORE_NUM
#endif

/** \brief Obtains the core index from the core information words.
 * \n core_id[0:7]: The core index. */
/* get core slice index from core ID */
#ifndef CORE
#define CORE(id) ((u32)(id)&0xff)
#endif
/** \brief Obtains the slice node index from the core information words.
 * \n core_id[16:23]: The slice node index. */
#ifndef NODE
#define NODE(id) ((u32)(id) >> 16)
#endif
/** \brief The derived core ID, which is the combination of the slice node index and the core index. */
#ifndef COREID
/* combine slice node and core index as COREID */
#define COREID(node, core) (((u32)(node) << 16) | ((u32)(core)&0xff))
#endif
/**@}*/

#ifdef WIN32
#define GETPID() _getpid()
#else
#define GETPID() getpid()
#endif

/**
 * \defgroup ewl_mem Memory Management
 *
 * @{
 */

#define MEM_CHUNKS 720 /**< \brief The maximum number of linear memory used for status only. */
/** \brief The page size and alignment for linear memory chunks. */
#define LINMEM_ALIGN 4096

/** \brief The nearest aligned linear memory address before the input address x. */
#define NEXT_ALIGNED(x) \
  ((((ptr_t)(x)) + LINMEM_ALIGN - 1) / LINMEM_ALIGN * LINMEM_ALIGN)
/** \brief The nearest address aligned with "align" before the input address x. */
#define NEXT_ALIGNED_SYS(x, align) ((((ptr_t)(x)) + align - 1) / align * align)
 /** \brief The memory accessed by CPU. It is a non-secure memory. */
#define EWL_MEM_TYPE_CPU 0x0000U
#define EWL_MEM_TYPE_SLICE 0x0001U /**< \brief The output buffer for the encoder. */
#define EWL_MEM_TYPE_DPB 0x0002U   /**< \brief The input buffer or reconstructed frame buffer for the encoder. */
/** \brief The working buffer for the encoder. It is a non-secure memory. */
#define EWL_MEM_TYPE_VPU_WORKING 0x0003U
#define EWL_MEM_TYPE_VPU_WORKING_SPECIAL 0x0004U /**< \brief VPU reads the memory; CPU reads and writes the memory. */
#define EWL_MEM_TYPE_VPU_ONLY 0x0005U            /**< \brief VPU reads and writes the memory. */

/** \brief The bit mask (bits [22:16]) that indicates the usage hint of the buffer. */
#define EWL_MEM_USAGE_HINT_MASK 0x7F0000U
/** \brief The least significant bit (LSB) of memory usage hint bits in memory attribute words. */
#define EWL_MEM_USAGE_HINT_SHIFT 16
/** \brief Bit 23, which indicates whether the memory attribute is META or CORE. */
#define EWL_MEM_ATTR_SHIFT 23
/** The security attribute of a buffer.*/
enum MEM_ATTR {
  META, /**< The buffer does not have sensitive data. */
  CORE  /**< The buffer has sensitive data. */
};
/** \brief Non-specified attributes of a memory. These attribute are regarded as META. */
#define ATTR_NOT_DEF META

/**@} */

#define EWL_HINT_LIST(DEF) \
  /** (Default) The attribute is not defined. */ \
  DEF(EWL_MEM_USAGE_UNDEFINED, ATTR_NOT_DEF) /* invalid */ \
  /** The attribute for an input picture buffer. */ \
  DEF(EWL_MEM_USAGE_IN_SURFACE, CORE) /* R1, */ \
  /** The attribute for a QP map buffer. */ \
  DEF(EWL_MEM_USAGE_IN_QPMAP, META)  /* R7, W64+ */  \
  /** The attribute for a buffer that stores overlay pictures. */ \
  DEF(EWL_MEM_USAGE_IN_OVERLAY, ATTR_NOT_DEF) /* R11 */ \
  /** The attribute for a CU control buffer. */ \
  DEF(EWL_MEM_USAGE_IN_CUCTLMAP, META) /* R7 */ \
  /** (Reserved) The attribute for a CU index map buffer. */ \
  DEF(EWL_MEM_USAGE_IN_CUIDXMAP, ATTR_NOT_DEF) /* test */ \
  /** The attribute for downscale input for pass-1 encoding when lookahead is enabled. */ \
  DEF(EWL_MEM_USAGE_IN_DOWNSACLE, CORE) /* R1 */ \
  /** The attribute for DEC400 tile status of input picture buffer. */ \
  DEF(EWL_MEM_USAGE_IN_SURFACE_DECTS, META) /* R31 */ \
  /** The attribute for DEC400 tile status of input overlay buffer. */ \
  DEF(EWL_MEM_USAGE_IN_OVERLAY_DECTS, META) /* R31 */ \
  /** (Reserved) The attribute for MV information. */ \
  DEF(EWL_MEM_USAGE_IN_MVINFO, ATTR_NOT_DEF) /* dev */ \
  /** (For test only) The attribute for input picture translation. */\
  DEF(EWL_MEM_USAGE_IN_TRANSFORM, ATTR_NOT_DEF) /* test */ \
  /** The attribute for a line buffer (as input picture) in low-latency mode. */ \
  DEF(EWL_MEM_USAGE_IN_LINEBUF, CORE) /* R1 */ \
  /** The attribute for OSD map buffer. */ \
  DEF(EWL_MEM_USAGE_IN_OSDMAP, ATTR_NOT_DEF) \
  /** The attribute for output stream. */ \
  DEF(EWL_MEM_USAGE_OUT_STRM, CORE) /* W4 */ \
  /** The attribute for output CU information.  */ \
  DEF(EWL_MEM_USAGE_OUT_CUINFO, META) /* W2 */ \
  /** The attribute for the buffer that stores the NAL unit size of output streams. */ \
  DEF(EWL_MEM_USAGE_OUT_SIZETABLE, META) /* W3 */ \
  /** The attribute for status buffer which have bits count of every CTB. */ \
  DEF(EWL_MEM_USAGE_OUT_CTBBITS, ATTR_NOT_DEF) /* dev */ \
  /** The attribute for downscale output picture. */ \
  DEF(EWL_MEM_USAGE_OUT_SCAL, CORE) /* W8 */ \
  /** The attribute for reference compression tables. */ \
  DEF(EWL_MEM_USAGE_TMP_RFCTS, CORE) /* R3, R4, R5, R6, R8, W5, W6, W7 */ \
  /** The attribute for scratch of an entropy engine. */ \
  DEF(EWL_MEM_USAGE_TMP_COEFF, META) /* R2, W2 */ \
  /** The attribute for scratch of tile context. */ \
  DEF(EWL_MEM_USAGE_TMP_TILECTX, ATTR_NOT_DEF) /* not configure */ \
  /** The attribute for scratch of tile edge. */ \
  DEF(EWL_MEM_USAGE_TMP_TILEHEIGHT, ATTR_NOT_DEF) /* not configure */ \
  /** The attribute for a buffer that stores reconstructed frames. */ \
  DEF(EWL_MEM_USAGE_TMP_RECON, CORE) /* R3, R4, R5, R6, R8, W5, W6, W7 */ \
  /** The attribute for a buffer used for CTBRC. */ \
  DEF(EWL_MEM_USAGE_TMP_CTBRC, META) /* R9, W9*/ \
  /** The attribute for multi-core synchronization words. */ \
  DEF(EWL_MEM_USAGE_TMP_MCSYNC, META) /* R10 R14, W10, W13*/ \
  /** The attribute for a TMVP buffer. */ \
  DEF(EWL_MEM_USAGE_TMP_TMVP, META) /* R15, W3 */ \
  /** The attribute for a cost buffer. */ \
  DEF(EWL_MEM_USAGE_TMP_COST, META) /* R64+, W64+ */ \
  /** The attribute for the external SRAM of ME. */ \
  DEF(EWL_MEM_USAGE_TMP_EXTSRAM, CORE) /* not configure */ \
  /** The attribute for colocate information for H264. */ \
  DEF(EWL_MEM_USAGE_TMP_H264COL, META) /* R2, W1 */ \
  /** The attribute for frame context for AV1. */ \
  DEF(EWL_MEM_USAGE_TMP_AV1FRMCTX, META) /* R13,W12 */ \
  /** The attribute for pre-carry buffer for AV1. */ \
  DEF(EWL_MEM_USAGE_TMP_AV1PRC, CORE) /* R12, W4 */ \
  /** The attribute for frame context for VP9. */ \
  DEF(EWL_MEM_USAGE_TMP_VP9FRMCTX, META) /* R13,W12 */ \
  /** The attribute for frame status output. */ \
  DEF(EWL_MEM_USAGE_TMP_FRAMEINFO, ATTR_NOT_DEF) /* not configure */ \
  /** The attribute for the slice information used in low-latency encoding DDR mode. */ \
  DEF(EWL_MEM_USAGE_TMP_SLICEINFO, META) /* R11 */ \
  DEF(EWL_MEM_USAGE_MAX, ATTR_NOT_DEF)    /* invalid */

#define DEF_ENUM(enum, attr) enum,

/**
 * \addtogroup ewl_mem
 *
 * @{
 */
/** Specifies the usage of a buffer. */
enum EWLMemUsageHint {
  EWL_HINT_LIST(DEF_ENUM)
};

/**@} */

#define DEF_ATTR(enum, attr) enum##_ATTR=attr,

/** Specifies the buffer attributes when the security mode is enabled. */
enum EWLMemAttribute {
  EWL_HINT_LIST(DEF_ATTR)
};

#define USAGE_ATTR(usage) usage##_ATTR

/**
 * \addtogroup ewl_mem Memory Management
 *
 * @{
 */
/** \brief Sets the memory usage hint and security flag in memory attribute words. For details, see Section <i>Macro Definition</i>.
 *
 * \param [inout] mem_type Input: The memory attribute words and other attributes. <br>
 *    Output: The input data added with usage hint and security flag.
 * \param [in] usage The memory usage hint.
 * \param [in] is_secure The security status of the memory.
 */
#define SET_MEM_USAGE(mem_type, usage, is_secure) do { \
      (mem_type) |= ((usage)<<EWL_MEM_USAGE_HINT_SHIFT | \
      ((USAGE_ATTR(usage)&is_secure)<<EWL_MEM_ATTR_SHIFT)); \
  } while(0)

/** \brief Specifies whether the memory is secure by parsing the memory attribute flag. */
#define EWLGetMemAttribute(mem_type) ((mem_type>>EWL_MEM_ATTR_SHIFT)&0x1)

#define CPU_RD 0x0100U /**< \brief CPU reads the memory.*/
#define CPU_WR 0x0200U /**< \brief CPU writes the memory.*/
#define VPU_RD 0x0400U /**< \brief VPU reads the  memory.*/
#define VPU_WR 0x0800U /**< \brief VPU writes the memory.*/
#define EXT_RD 0x1000U /**< \brief External IP reads the memory.*/
#define EXT_WR 0x2000U /**< \brief External IP writes the memory.*/

 /** \brief The provided parameter is wrong. */
#define EWL_PAR_ERROR EWL_ERROR
 /** \brief Errors occur, when the specified function runs. */
#define EWL_HW_ERROR -2

/** Specifies the data transfer direction of EWL. */
enum EWLMemSyncDirection {
  HOST_TO_DEVICE = 0, /**< Copies data from the host memory to the device memory. */
  DEVICE_TO_HOST = 1  /**< Copies data from the device memory to the host memory. */
};

#define UFBC_HW_ID_1_1_0 0x42431000
#define UFBC_HW_ID_1_1_1 0x42431100

/** \brief Specifies allocated memory information for counting. */
typedef struct EWLMemoryStatistics {
  u32 maxHeapMemory;      /**< \brief The maximum memory value. */
  u32 memoryValue;        /**< \brief Memory value. */
  u32 maxHeapMallocTimes; /**< \brief The number of times <tt>EWLmalloc()</tt> or <tt>EWLcalloc()</tt> are called. */
  u32 averageHeapMallocTimes; /**< \brief The maximum heapMallocTimes. \n For example, if <tt>EWLmalloc()</tt>
  is called 10 times and <tt>EWLfree()</tt> is called twice, <tt>averageHeapMallocTimes</tt> and <tt>heapMallocTimes</tt> are eight.
  If then, <tt>EWLfree()</tt> is called one more time, <tt>heapMallocTimes</tt> is seven, but <tt>averageHeapMallocTimes</tt>
  is still eight. On the contrary, if <tt>EWLmalloc()</tt> is called one more time, <tt>HeapMallocTimes</tt> becomes nine,
  larger than the former eight, and <tt>maxHeapMallocTimes</tt> becomes nine as well. */
  u32 heapMallocTimes; /**< \brief The number of times [<tt>EWLmalloc()</tt> - <tt>EWLfree()</tt>] are called. */

  u32 maxLinearMemory;   /**< \brief The maximum memory value of a linear buffer. */
  u32 linearMemoryValue; /**< \brief The memory value of linear buffer. */
  /** \brief The number of linear buffers allocated. The number is counted by how many times
   * <tt>EWLMallocLinear()</tt> or <tt>EWLMallocRefFrm()</tt> are called. */
  u32 maxLinearMallocTimes;
  /** \brief The maximum <tt>linearMallocTimes</tt>. \n For example, if <tt>EWLMallocLinear()</tt> is called ten times,
   * and <tt>EWLFreeLinear()</tt> is called twice, <tt>averageLinearMallocTimes</tt> and <tt>linearMallocTimes</tt> are eight.
   * If then, <tt>EWLFreeLinear()</tt> is called one more time, <tt>linearMallocTimes</tt> is seven,
   * but <tt>maxLinearMallocTimes</tt> is still eight. On the contrary, if <tt>EWLMallocLinear()</tt> is called one more time,
   * <tt>linearMallocTimes</tt> becomes nine, larger than the former eight, and <tt>maxLinearMallocTimes</tt> becomes nine as well. */
  u32 averageLinearMallocTimes;
  /** \brief The number of linear buffers that are not released when an instance is released.
   * This number is counted by subtracting the number of times <tt>EWLMallocLinear()</tt> is called from
   * the number of times <tt>EWLFreeLinear()</tt> is called.*/
  u32 linearMallocTimes;
} EWLMemoryStatistics_t;

/** \brief The information of the memory allocated as a linear buffer. */
typedef struct EWLLinearMem {
  u32 *virtualAddress;   /**< \brief The aligned virtual address of the memory for CPU to access. */
  ptr_t busAddress;      /**< \brief The aligned bus address of the memory for VPU to access. */
  u32 size;              /**< \brief The size of memory, in bytes. */
  u32 *allocVirtualAddr; /**< \brief The original virtual address of the memory for CPU to access. */
  ptr_t allocBusAddr;    /**< \brief The original bus address of the memory for VPU to access. */
  unsigned long id;      /**< \brief The ID of the buffer. */
  u32 mem_type;          /**< \brief The owner and attributes of the memory.
                          * \n Bit 23 indicates whether the buffer has sensitive data. See @ref MEM_ATTR.
                          * \n Bits [22:16] indicate the usage hints of memory. See @ref EWLMemUsageHint.
                          * \n Bits [15:8] indicate which IP is allowed to access the memory.
                          * \n Bits [7:0] indicate the attributes of the memory.*/
  u32 total_size;        /**< \brief The size of the allocated buffer, in bytes. */
  void *priv;            /**< \brief Customer-defined field for application use. */
} EWLLinearMem_t;

/**@} */

#ifdef MEM_ONLY_DEV_CHECK
 /** \brief Specifies whether the bus address of a linear memory is set. */
#define EWL_DEVMEM_VAILD(mem) ((mem).busAddress != 0)
 /** \brief reset the bus address of a linear memory as zero. */
#define EWL_CLEAN_DEVMEM_ADDR(mem) ((mem).busAddress = 0)
#else
 /** \brief Specifies whether the virtual address of a linear memory is set. */
#define EWL_DEVMEM_VAILD(mem) ((mem).virtualAddress != NULL)
 /** \brief reset the virtual address of a linear memory as NULL. */
#define EWL_CLEAN_DEVMEM_ADDR(mem) ((mem).virtualAddress = NULL)
#endif

/**
 * \defgroup ewl_api EWL API
 *
 * @{
 */

/** \brief The parameters required for EWL initialization. */
typedef struct EWLInitParam {
  /** \brief The encoding format. See @ref CLIENT_TYPE. */
  u32 clientType;
  /** \brief The context used for initialization. The context is sent to the application by EWL. */
  void *context;
  /** \brief The index of the slice node for multi-node VPU. \n This field is valid only for specific versions. */
  u32 slice_idx;
  /** \brief Specifies whether the hardware supports MMU. \n 0: Does not support. \n 1: Supports. */
  u32 mmuEnable;
  /** \brief The device name of the enc driver. */
  char *enc_dev;
  /** \brief The device name of the memalloc driver. */
  char *mem_dev;
  /** \brief Specifies whether to use VCMD. \n 0: Not use. \n 1: Use. */
  i32 useVcmd;
} EWLInitParam_t;

/** \brief The L2Cache. */
typedef struct CacheData {
  void **cache; /**< \brief The L2Cache. */
} CacheData_t;

typedef void (*CoreWaitCallBackFunc)(const void *ewl, void *data);

/** \brief The information of a job  */
typedef struct EWLCoreWaitJob {
  struct node *next; /**< \brief The next job. */
  u32 id;            /**< \brief The ID of the job the structure describes. */
  u32 core_id;       /**< \brief The index of the hardware subsystem. */
  const void *inst;  /**< \brief The <tt>vcenc_instance</tt> structure. */
  /** \brief All hardware registers. <tt>ASIC_SWREG_AMOUNT</tt> indicates the maximum number of hardware registers. */
  u32 VCE_reg[ASIC_SWREG_AMOUNT];
  i32 out_status; /**< \brief The interrupt status of the job. \n The value can be:
                               \n - @ref ASIC_STATUS_SEGMENT_READY
                               \n - @ref ASIC_STATUS_FUSE_ERROR
                               \n - @ref ASIC_STATUS_SLICE_READY
                               \n - @ref ASIC_STATUS_LINE_BUFFER_DONE
                               \n - @ref ASIC_STATUS_HW_TIMEOUT
                               \n - @ref ASIC_STATUS_BUFF_FULL
                               \n - @ref ASIC_STATUS_HW_RESET
                               \n - @ref ASIC_STATUS_ERROR
                               \n - @ref ASIC_STATUS_FRAME_READY */
  i32 out_poll_sliceinfo_status; /**< \brief The status indicating whether timeout occurs when polling sliceinfo. */
  i32 out_ufbc_status; /**< \brief the interrupt status of ufbc, mainly for ASIC_STATUS_UFBC_DEC_ERR */
  u32 slices_rdy;     /**< \brief The number of completed slices. */
  u32 low_latency_rd; /**< \brief The number of CTB rows that the encoder fetches from the input buffer for the job. */
  u32 dec400_enable;  /**< \brief The status of DEC400. \n <tt>1</tt>: Bypassed.  \n <tt>2</tt>: Enabled. */
  //VCDec400data dec400_data;
  /** \brief (Reserved) The DEC400 callback function. */
  CoreWaitCallBackFunc dec400_callback;
  /** \brief Pointer to the DEC400 data. */
  void *dec400_data;
  u32 axife_enable; /**< \brief The status of AXI_FE. \n <tt>0</tt>: Disabled.  \n <tt>1</tt>: Enabled.  \n <tt>2</tt>: Bypassed.  \n <tt>3</tt>: Security mode.*/
  /** \brief The AXI_FE callback function. */
  CoreWaitCallBackFunc axife_callback;
  u32 l2cache_enable; /**< \brief The status of L2Cache. \n <tt>0</tt>: Disabled. \n <tt>1</tt>: Enabled. */
  CacheData_t l2cache_data; /**< \brief The L2Cache. */
  /** \brief The L2Cache callback function. */
  CoreWaitCallBackFunc l2cache_callback;
  /** \brief Whether to enable UFBC. \n
             <tt>0</tt>: Disable. \n
             <tt>1</tt>: Enabled. */
  u32 ufbcEnable;
  /** \brief The UFBC callback. See <tt>EncUfbcAsicStop</tt>. */
  CoreWaitCallBackFunc ufbc_callback;
} EWLCoreWaitJob_t;

/** \brief Parameters for jobs to be processed in a thread. */
typedef struct EWLCoreWait {
  struct queue jobs;         /**< \brief The queue of unfinished jobs, including jobs to be processed and jobs under processing. */
  vsios_mutex_t job_mutex; /**< \brief The mutex for protecting job_pool. */
  vsios_sem_t job_cond;    /**< \brief The condition variable for job_mutex (implemented as semaphore). */

  /** \brief The queue of all completed jobs, including abnormal and normal ones. */
  struct queue out;
  vsios_mutex_t out_mutex; /**< \brief The mutex for protecting the out queue. */
  vsios_sem_t out_cond;    /**< \brief The condition variable for out_mutex (implemented as semaphore). */
  vsios_thread_t thread; /**< \brief The thread. */
  bool bFlush;               /**< \brief Whether the last job is queued. \n <tt>0</tt>: No.  \n <tt>1</tt>: Yes. */

  /** \brief The reference frame counter for encoding types of HEVC, H264, AV1, and VP9.
   * \n When the EWL instance is released, the field value equals 0. */
  u32 refer_counter;
  /** \brief The pool that stores jobs. */
  struct queue job_pool;
} EWLCoreWait_t;

/** \brief The job configuration. */
typedef struct EWLWaitJobCfg {
  u32 waitCoreJobid; /**< \brief The job ID maintained by the kernel driver. */
  u32 dec400_enable; /**< \brief The status of DEC400. \n <tt>1</tt>: Bypassed. \n <tt>2</tt>: Enabled. */
  void *dec400_data; /**< \brief The DEC400. */
  CoreWaitCallBackFunc dec400_callback; /**< \brief dec400 callback resvered for furture */
  /** \brief The status of AXI_FE. \n
             <tt>0</tt>: Disabled. \n
             <tt>1</tt>: Enabled. \n
             <tt>2</tt>: Bypassed. \n
             <tt>3</tt>: Security mode. */
  u32 axife_enable;
  /** \brief The AXI_FE callback function. */
  CoreWaitCallBackFunc  axife_callback;
  u32 l2cache_enable; /**< \brief The status of L2Cache. \n <tt>0</tt>: Disabled. \n <tt>1</tt>: Enabled. */
  void *l2cache_data; /**< \brief The L2Cache. */
  /** \brief The L2Cache callback function. */
  CoreWaitCallBackFunc l2cache_callback;
  /** \brief The Status of UFBC. \n
             <tt>0</tt>: Disabled. \n
             <tt>1</tt>: Enabled. */
  u32 ufbcEnable;
  /** \brief The UFBC callback. See <tt>EncUfbcAsicStop</tt>. */
  CoreWaitCallBackFunc ufbc_callback;
} EWLWaitJobCfg_t;

/** \brief The information about a VCMD buffer, when reserving buffer resources. */
typedef struct {
  u16 size;         /**< \brief [in] The buffer size to reserve. */
  u16 id;           /**< \brief [out] The ID of the VCMD buffer. */
  ptr_t status_ba;  /**< \brief [out] The bus address of the status buffer. */
  u32 *cmdbuf_va;   /**< \brief [out] The virtual address of the VCMD buffer. */
  u32 core_mask;    /**< \brief [in] The mask that indicates which core is selected. */
  u32 priority;     /**< \brief [in] The priority of the selected VCMD buffer. */
} EWLResourceVcmdBuf;

/** \brief Information about reserved resources. */
typedef union EWLResource {
  /** \brief The information about the VCMD buffer, when reserving buffer resources. */
  EWLResourceVcmdBuf vcmdbuf;
} EWLResource_t;

/** The hardware engine type. */
typedef enum {
  /** Video or image encoder main core */
  EWL_CLIENT_TYPE_MAIN = 0U,
  /** H.264 encoder */
  EWL_CLIENT_TYPE_H264_ENC = 0U,
  /** HEVC encoder */
  EWL_CLIENT_TYPE_HEVC_ENC = 1U,
  /** VP9 encoder */
  EWL_CLIENT_TYPE_VP9_ENC = 2U,
  /** JPEG encoder */
  EWL_CLIENT_TYPE_JPEG_ENC = 3U,
  /** CuTree analyzer engine */
  EWL_CLIENT_TYPE_CUTREE = 4U,
  /** (Reserved) Video stabilization engine */
  EWL_CLIENT_TYPE_VIDEOSTAB = 5U,
  /** DEC400 engine */
  EWL_CLIENT_TYPE_DEC400 = 6U,
  /** AV1 encoder */
  EWL_CLIENT_TYPE_AV1_ENC = 7U,
  /** L2Cache engine */
  EWL_CLIENT_TYPE_L2CACHE = 8U,
  /** AXI_FE engine */
  EWL_CLIENT_TYPE_AXIFE = 9U,
  /** APB filter engine */
  EWL_CLIENT_TYPE_APBFT = 10U,
  /** AXI_FE_1 engine */
  EWL_CLIENT_TYPE_AXIFE_1 = 11U,
  /** memalloc */
  EWL_CLIENT_TYPE_MEM = 12U,
  /** MMU0 */
  EWL_CLIENT_TYPE_MMU0 = 13U,
  /** MMU1 */
  EWL_CLIENT_TYPE_MMU1 = 14U,
  /** UFBC */
  EWL_CLIENT_TYPE_UFBC = 15U,
  EWL_CLIENT_TYPE_MAX
} CLIENT_TYPE;

/** \brief Specifies whether the client type is HEVC */
#define EWL_IS_HEVC_CLIENT(clientType)  ((clientType) == EWL_CLIENT_TYPE_HEVC_ENC)
/** \brief Specifies whether the client type is H.264. */
#define EWL_IS_H264_CLIENT(clientType)  ((clientType) == EWL_CLIENT_TYPE_H264_ENC)
/** \brief Specifies whether the client type is AV1. */
#define EWL_IS_AV1_CLIENT(clientType)   ((clientType) == EWL_CLIENT_TYPE_AV1_ENC)
/** \brief Specifies whether the client type is VP9. */
#define EWL_IS_VP9_CLIENT(clientType)   ((clientType) == EWL_CLIENT_TYPE_VP9_ENC)
/** \brief Specifies whether the client type is JPEG. */
#define EWL_IS_JPEG_CLIENT(clientType)  ((clientType) == EWL_CLIENT_TYPE_JPEG_ENC)
/** \brief Specifies whether the client type is CuTree. */
#define EWL_IS_CUTREE(clientType)       ((clientType) == EWL_CLIENT_TYPE_CUTREE)
/** \brief Specifies whether the client type is video stabilization. */
#define EWL_IS_VIDEOSTAB(clientType)    ((clientType) == EWL_CLIENT_TYPE_VIDEOSTAB)
/** \brief Specifies whether the client is for video encoding. For details, see Section <i>Macro Definition</i>. */
#define EWL_IS_VIDEO_CLIENT(clientType) ( EWL_IS_HEVC_CLIENT(clientType) \
	    || EWL_IS_H264_CLIENT(clientType) \
        || EWL_IS_AV1_CLIENT(clientType) \
        || EWL_IS_VP9_CLIENT(clientType))
/** \brief Specifies whether the given client type is supported and enabled. For details, see Section <i>Macro Definition</i>. */
#define EWL_SUPPORT_CLIENT(cfg, clientType) \
        (((cfg->hevcEnabled == 1) && EWL_IS_HEVC_CLIENT(clientType)) \
        || ((cfg->h264Enabled == 1) && EWL_IS_H264_CLIENT(clientType)) \
        || ((cfg->av1Enabled == 1) && EWL_IS_AV1_CLIENT(clientType)) \
        || ((cfg->vp9Enabled == 1) && EWL_IS_VP9_CLIENT(clientType)) \
        || ((cfg->jpegEnabled == 1) && EWL_IS_JPEG_CLIENT(clientType)) \
        || ((cfg->vsSupport == 1) && EWL_IS_VIDEOSTAB(clientType)))
/** \brief Specifies whether the client is CuTree and whether it is supported. */
#define EWL_SUPPORT_CUTREE(cfg, clientType) \
        ((cfg->cuTreeSupport == 1) && EWL_IS_CUTREE(clientType))

extern u32 (*pollInputLineBufTestFunc)(void);

/*------------------------------------------------------------------------------
      4.  Function prototypes
  ------------------------------------------------------------------------------*/

/**@} */

/**
 * \addtogroup ewl_info
 *
 * @{
 */
/** Obtains the hardware ID of the specified core.
 * \param [in] core_id The ID to identify the core.
 * \param [in] ctx The context used to access the hardware, for example, the device description for this instance.
 * \return The hardware ID of the specified core.
 */
u32 EWLReadAsicID(u32 core_id, const void *ctx);

/** Obtains the core number.
 * \param [in] ctx The context used to access the hardware, for example, the device description for this instance.
 * \return The core number in the system.
 */
u32 EWLGetCoreNum(const void *ctx);

/** Specifies whether the CuTree hardware is available in the system.
 * \param [in] inst An EWL instance.
 * \return <tt>EWL_ERROR</tt>
 * \return <tt>EWL_OK</tt>
 */
i32 EWLCheckCutreeValid(const void *inst);

/** Obtains the DEC400 attribute for specified input pixel format.
 * \param [in] pixel_format The picture pixel format, which is compressed by DEC400, in the buffer.
 * \param [out] tile_size A pointer to the tile size of the corresponding pixel format.
 * \param [out] bits_tile_in_table A pointer to the bits used in the table for one tile.
 * \param [out] planar420_cbcr_table_style A pointer to the chroma table attribute.
 */
void EWLGetDec400Attribute(u32 *tile_size,
                           u32 *bits_tile_in_table,
                           u32 *planar420_cbcr_table_style);

/** Obtains hardware configuration information.
 * \param [in] core_id The ID to identify the core.
 * \param [in] ctx The context used to access the hardware, for example, the device description for this instance.
 * \return The hardware configuration of the specified core.
*/
const EWLHwConfig_t *EWLReadAsicConfig(u32 core_id, const void *ctx);

/**@} */

/**
 * \addtogroup ewl_api
 *
 * @{
 */
/** Obtains the core ID of a core, to which the DEC400 is attached.
 * \param [in] inst An EWL instance.
 * \return The index of the subsystem.
 */
i32 EWLGetDec400Coreid(const void *inst);

/** Obtains the VCMD hardware version ID.
 * \param [in] inst An EWL instance.
 * \return The version ID of the VCMD.
 */
u32 EWLGetVcmdVersionId(const void *inst);

/** Maps the registers for access from user space.
 * \param [in] ewl An EWL instance.
 * \return <tt>0</tt>: Mapping succeeds.
 * \return <tt>-1</tt>: Mapping fails.
 */
int MapAsicRegisters(void *ewl);

/** Obtains the client type used to initialize the instance.
 * \param [in] inst An EWL instance.
 * \return The client type.
 */
u32 EWLGetClientType(const void *inst);

/** Converts the client type to core type.
 * \param [in] client_type The client type to write.
 * \return The core type.
 */
u32 EWLGetCoreTypeByClientType(u32 client_type);

/** Initializes an EWL instance.
 * \param [in] param The configuration for this instance.
 * \return An EWL instance, or NULL in case of function failure.
 */
const void *EWLInit(EWLInitParam_t *param);

/** Releases an EWL instance.
   * \return <tt>EWL_OK</tt>
   * \return <tt>EWL_ERROR</tt>
   */
i32 EWLRelease(const void *inst);

/** Reserves the hardware resources for the current EWL instance.
 *
 * This function should be called after the software prepared a buffer and register values for encoding one frame and the hardware can start encoding.
 * It is blocked if resources are unavailable.
 * After the hardware resources are reserved, the software can access these resources directly.
 *
 * \param [inout] core_info The information to specify which core or cores can be used for the current encoding.
 * \param [out] job_id The ID used to note that the current frame encoding job is filled.
 * \return <tt>EWL_OK</tt>
 * \return <tt>EWL_ERROR</tt>
 */
i32 EWLReserveHw(const void *inst, u32 *core_info, u32 *job_id);

/** Releases the hardware resources.
 *
 * This function should be called after the hardware finishes encoding one frame and the software has
 * obtained the encoding information from hardware registers. After calling this function, the software
 * releases the hardware resources reserved for the current instance. Then, other instances can access
 * these hardware resources.
 *
 * The software can only access hardware resources, such as registers, between calling
 * <tt>EWLReserveHw()</tt> and <tt>EWLReleaseHw()</tt>.
 */
void EWLReleaseHw(const void *inst);

/** Obtains the number of hardware cycles used for encoding one frame.
 *
 * The cycles are counted according to the encoder clock.
 *
 * \return The number of hardware cycles used for encoding one frame.
 */
u32 EWLGetPerformance(const void *inst);

/**@} */

void EwlReleaseCoreWait(void *inst);

EWLCoreWaitJob_t *EWLDequeueCoreOutJob(const void *inst, u32 waitCoreJobid);

void EWLEnqueueOutToWait(const void *inst, EWLCoreWaitJob_t *job);

void EWLEnqueueWaitjob(const void *inst, EWLWaitJobCfg_t *cfg);

void EWLPutJobtoPool(const void *inst, struct node *job);

/**
 * \addtogroup ewl_mem
 *
 * @{
 */
/** Allocates a linear memory for reference frames. Reference frames are only accessed by hardware.
 * \param [in] size The bytes to be allocated.
 * \param [in] alignment The required alignment of the buffer.
 * \param [out] info The information of the memory allocated as a linear buffer.
 */
i32 EWLMallocRefFrm(const void *instance, u32 size, u32 alignment,
                    EWLLinearMem_t *info);

/** Releases the linear buffer allocated by EWLMallocRefFrm().
 *
 * \param [in] info The information to identify the buffer to be released.
 */
void EWLFreeRefFrm(const void *inst, EWLLinearMem_t *info);

/** Specifies software/hardware-shared buffer without memory synchronization.
 *
 * When MMU is disabled, this buffer should be physically continuous.
 *
 * \param [in] size The bytes to be allocated.
 * \param [in] alignment The required alignment of the buffer.
 * \param [out] info The information of the memory allocated as a linear buffer.
 */
i32 EWLMallocLinearExe(const void *instance, u32 size, u32 alignment,
                    EWLLinearMem_t *info);

//FIXME@DBG
#define EWLMallocLinear(a,b,c,d) EWLMallocLinearExe(a,b,c,d)
//#define EWLMallocLinear(a,b,c,d) (EWLMallocLinearExe(a,b,c,d) + bk_printf("%s:%d\n", __FILE__, __LINE__) / 10000)

/** Releases the linear memory allocated by <tt>EWLMallocLinear()</tt>.
 *
 * \param [in] info The information to identify the buffer to be released.
 */
void EWLFreeLinear(const void *inst, EWLLinearMem_t *info);

/**@} */

/* D-Cache coherence flush (obsolete) */ /* Not in use currently */
void EWLDCacheRangeFlush(const void *instance, EWLLinearMem_t *info);
/* D-Cache coherence refresh (obsolete) */ /* Not in use currently */
void EWLDCacheRangeRefresh(const void *instance, EWLLinearMem_t *info);

/**
 * \addtogroup ewl_api
 *
 * @{
 */
/**
 * Writes a value to a hardware register.
 *
 * All registers are written before the hardware is enabled for encoding.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 */
void EWLWriteReg(const void *inst, u32 offset, u32 val);

/** Writes back a value to a hardware register.
 *
 * This function is called when a callback is completed, or a frame encoding in a multi-core scenario is completed.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 */
void EWLWriteBackReg(const void *inst, u32 offset, u32 val);

/** Writes a value to a hardware register by the specified core ID.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 * \param [in] core_id The ID to specify the core to write.
 */
void EWLWriteCoreReg(const void *inst, u32 offset, u32 val, u32 core_id);

/** Writes a value to a hardware register by the specified client type.
 *
 *  This function is often used to access peripheral registers of sub-IPs.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 * \param [in] client_type The client type to write.
 */
void EWLWriteRegbyClientType(const void *inst, u32 offset, u32 val,
                             u32 client_type);

/** Writes back a value to a hardware register by the specified client type.
 *
 *  This function is often used to access peripheral registers of sub-IPs.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] val The value to be written into the register.
 * \param [in] client_type The client type to write.
 */
void EWLWriteBackRegbyClientType(const void *inst, u32 offset, u32 val,
                                 u32 client_type);

/** Obtains the value in a status register.
 *
 * Such registers are updated by the hardware and are read after an interrupt is generated.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \return The value of the hardware register.
 */
u32 EWLReadReg(const void *inst, u32 offset);

/** Obtains the value in a status register of one client.
 *
 * Such registers are updated by the hardware and are read after an interrupt is generated.
 *
 * This function is often used to access peripheral registers of sub-IPs.
 *
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \return The value of the hardware register.
 */
u32 EWLReadRegbyClientType(const void *inst, u32 offset, u32 client_type);

/* Writing all registers in one call */ /* Not in use currently */
void EWLWriteRegAll(const void *inst, const u32 *table, u32 size);
/* Reading all registers in one call */ /* Not in use currently */
void EWLReadRegAll(const void *inst, u32 *table, u32 size);

/** Enables the hardware to start encoding a frame.
 * \param [in] offset The offset related to the hardware register base address, in bytes. Its value should be set to 0x14.
 * \param [in] val The value to be written into the register. Bit 0 of this parameter should be 1.
 */
i32 EWLEnableHW(const void *inst, u32 offset, u32 val);

/** Disables the hardware and stops encoding.
 * \param [in] offset The offset related to the hardware register base address, in bytes. Its value should be set to 0x14.
 * \param [in] val The value to be written into the register. Bit 0 of this parameter should be 0.
 */
void EWLDisableHW(const void *inst, u32 offset, u32 val);

/**
 * Waits for the hardware to complete encoding.
 *
 * After the hardware is enabled, this function is called to wait for frame encoding completion or error interrupts.
 * In polling mode, the software polls the hardware status register to check whether there is any hardware status change.
 * In interrupt mode, the software is in "sleep" status and waits for IRQ.
 * The final software product works in interrupt mode.
 *
 * \param [out] slicesReady The value saved in this pointer contains the number of encoded slices in the hardware output buffer.
 * \param [out] status_register The value saved in this pointer contains the bit map, which indicates the status of the encoding work.
 * \return <tt>EWL_HW_WAIT_OK</tt>
 * \return <tt>EWL_HW_WAIT_ERROR</tt>
 * \return <tt>EWL_HW_WAIT_TIMEOUT</tt>
 */
i32 EWLWaitHwRdy(const void *instance, u32 *slicesReady, void *waitOut,
                 u32 *status_register);


/**@} */

/**
 * \addtogroup ewl_mem
 *
 * @{
 */

/** Allocates n bytes and returns a pointer to the allocated memory.
 *
 *  This function is a wrapper of <tt>malloc()</tt>.
 *
 * \param [in] n The number of bytes to be allocated.
 * \return The address of the allocated memory.
 */
void *EWLmallocExe(u32 n);

/** Allocates memory for an array of n elements of s bytes each and returns a pointer to the allocated memory. The memory is set to zero.
 *
 * This function is a wrapper of <tt>calloc()</tt>.
 *
 * \param [in] n The number of elements.
 * \param [in] s The size of each element, in bytes.
 * \return The address of the allocated memory.
 */
void *EWLcallocExe(u32 n, u32 s);

//FIXME@DBG
#define EWLmalloc(n)    EWLmallocExe(n)
#define EWLcalloc(n, s) EWLcallocExe(n, s)
//#define EWLmalloc(n)    (EWLmallocExe(n) + bk_printf("%s:%d\n", __FILE__, __LINE__) / 1000)
//#define EWLcalloc(n, s) (EWLcallocExe(n, s) +  bk_printf("%s:%d\n", __FILE__, __LINE__) / 1000)

/**  Releases the allocated memory.
 *
 * This function is a wrapper of <tt>free()</tt>.
 */
void EWLfree(void *p);

/** Copies n bytes from memory area s to memory area d.
 *
 * This function is a wrapper of <tt>memcpy()</tt>.
 * \param [in] d The destined memory area for the copied bytes.
 * \param [in] s The source memory area where data values are copied.
 * \param [in] n The size of the copied memory area, in bytes.
 */
mem_ret EWLmemcpy(void *d, const void *s, u32 n);

/** Fills memory with a constant byte.
 *
 * This function is a wrapper of <tt>memset()</tt>.
 * \param [in] d The start memory address where the value is filled.
 * \param [in] c The byte value to be filled.
 * \param [in] n The number of bytes to be filled.
 * */
mem_ret EWLmemset(void *d, i32 c, u32 n);

/** Compares two memory areas.
 *
 * This function is a wrapper of <tt>memcmp()</tt>.
 * \param [in] s1 One memory area to be compared.
 * \param [in] s2 Another memory area to be compared.
 * \param [in] n The size of s1 and s2, in bytes.
 * */
int EWLmemcmp(const void *s1, const void *s2, u32 n);

/** Obtains the information, including address and size, of the input line buffer.
 * \param [in] info Information to identify the input line buffer.
*/
i32 EWLGetLineBufSram(const void *instance, EWLLinearMem_t *info);

/** Allocates memory for the loopback line buffer.
 * \param [in] size The bytes to be allocated.
 * \param [out] info Information to identify the loopback line buffer.
*/
i32 EWLMallocLoopbackLineBuf(const void *instance, u32 size,
                             EWLLinearMem_t *info);


/**@} */

/**
 * \addtogroup ewl_api
 *
 * @{
 */

/**
 * Obtains the register offset of a hardware engine in the subsystem.
 * \param inst An EWL instance.
 * \param client_type The hardware engine.
 *
 * \return  A value equal to 0xffff: the hardware engine inquired is not available.
 * \return A value smaller than 0xffff: the inquired hardware engine is detected and the value equals the register offset
 * of the hardware engine, in bytes.
 */
u16 EWLGetClientOffset(const void *inst, u16 client_type);

/**
 * Obtains the submodule register offset of status cmdbuf in the subsystem.
 * \param inst An EWL instance.
 * \param client_type The hardware engine.
 *
 * \return  A value equal to 0xffff: the hardware engine inquired is not available.
 * \return A value smaller than 0xffff: the inquired hardware engine is detected and the value equals the submodule register offset
 * of status cmdbuf, in bytes.
 */
u16 EWLGetClientVcmdStatusBufOffset(const void *inst, u16 comp_type);

/*
 * Obtains the PSNR and SSIM result. (Only available in the C-model now)
 * \param [in] prof_data Profile data.
 * \param [in] qp Slice quantization parameters used to encode the frame.
 * \param [in] poc Picture order count of the encoded frame.
 */
void EWLTraceProfile(const void *inst, void *prof_data, i32 qp, i32 poc);

/** Obtains the read-only registers loaded by VCMD during initialization.
 * \param [in] offset The offset related to the hardware register base address.
 * \return <tt>EWL_OK</tt> when VCMD is disabled.
 * \return The register values obtained during the VCMD initialization.
 */
u32 EWLReadRegInit(const void *inst, u32 offset);

/** Reserves one VCMD buffer for the current instance.
 *
 * This function requests a VCMD buffer from a system such as the kernel driver.
 * When this function returns, the software fills the buffer with commands to set up the encoding parameters for frame encoding.
 * This function waits internally untill there is a free VCMD buffer available.
 *
 * \param [inout] data
 *   - The <tt>data->vcmdbuf.size</tt> is an input parameter indicating the size required by one VCMD buffer. \n
 *   - The <tt>data->vcmdbuf.id</tt> is filled with the ID of the available VCMD buffer. \n
 *   - The <tt>data->vcmdbuf.status_ba</tt> is filled with the address where the register values read from the hardware are stored.
 * \return <tt>EWL_OK</tt>
 * \return <tt>EWL_ERROR</tt>
 */
i32 EWLReserveCmdbuf(const void *inst, EWLResource_t *data);
/** Puts a VCMD buffer into queue.
 *
 * When a queue is empty, the VCMD buffer is run at once. Otherwise, the VCMD buffer is linked to the end of the queue.
 *
 * \param [in] cmdbufid  The ID to identify the VCMD buffer.
 * \param [in] cmdbuf_size The size of the VCMD buffer.
 * \return <tt>EWL_ERROR</tt>
 * \return <tt>EWL_OK</tt>
 */
i32 EWLLinkRunCmdbuf(const void *inst, u16 cmdbufid, u16 cmdbuf_size);

/** Waits until all commands in the VCMD buffer obtained by <tt>EWLReserveCmdbuf()</tt> are executed.
 *
 * After the VCMD buffer is put into the VCMD buffer queue or run directly by calling <tt>EWLLinkRunCmdbuf()</tt>,
 * this function is called to wait for all commands being executed.
 * Such waiting can be asynchronous, awakened by the IRQ triggered by a VCMD interrupt.
 * It can also be synchronous to poll the VCMD status register.
 *
 * \param [in] cmdbufid The ID to identify the VCMD buffer to run.
 * \param [out] status The value stored in this pointer contains the bit map, which indicates the status of the encoding job.
 * \param [out] slices_rdy The value that indicates how many encoded slices are filled to this address.
 *
 * \return <tt>EWL_OK</tt>
 * \return <tt>EWL_ERROR</tt>
 * \return <tt>EWL_HW_WAIT_ERROR</tt>
 */
i32 EWLWaitCmdbuf(const void *inst, u16 cmdbufid, u32 *status);

/** Copies register values from a VCMD status buffer to mirror registers.
 *
 * \param [in] cmdbufid The ID to identify the VCMD buffer.
 * \param [out] regMirror The address of a buffer to save the register values.
 */
void EWLGetRegsByCmdbuf(const void *ewl, u16 cmdbufid, u32 *regMirror);

/** Releases the VCMD buffer used for encoding the current frame.
 *
 * This function should be called after the hardware finishes processing one VCMD Buffer and
 * software has gotten the encoding information from VCMD status buffer. After calling this API,
 * The VCMD buffer reserved for currrent instance will be returned to system. Then other instance
 * can use it. Software can only access the VCMD buffer between <tt>EWLReserveCmdbuf()</tt> and <tt>EWLReleaseCmdbuf()</tt>.
 *
 * \param [in] cmdbufid The ID to identify the VCMD buffer used for encoding.
 */
i32 EWLReleaseCmdbuf(const void *inst, u16 cmdbufid);

/** Sets the information about the commands stored in the VCMD Buffer.
 *
 * This function is used in VCMD mode only. The information helps the driver
 * estimate the workload and schedule the job in the VCMD Buffer. In parmeter
 * interrupt_ctrl, bit[31] is a flag used to indicate mode.
 * - When mode is 0, interrupt is generated adaptively. In such mode,
 *   - bit[30:0] is the estimating executing time for current job.
 * - When mode flag is 1, interrupt is generated according to bit[0].
 *   - When bit[0] is 0, VCMD will not generate interrupt when run JMP command;
 *   - when bit[0] is 1, VCMD will generate interrupt when run JMP command.
 *
 * \param [in] interrupt_ctrl control how to generate interrupt when execute
 *           VCMD JMP command.
 * \param [in] client_type The hardware engine type.
 * \param [in] priority The priority for the current job.
 */
void EWLSetReserveBaseData(const void *inst, u32 interrupt_ctrl,
                           u32 client_type, u16 priority);

/** Specifies whether VCMD is enabled.
 * \return <tt>1</tt>: Enabled.
 * \return <tt>0</tt>: Disabled.
 */
u32 EWLGetVCMDSupport(const void *inst);


/**@}*/

void EWLSetVCMDMode(const void *inst, u32 mode);
u32 EWLGetVCMDMode(const void *inst);


void EWLAttach(const void *ctx, int slice_idx, i32 vcmd_support);
void EWLDetach();

u32 EWLReleaseEwlWorkerInst(const void *inst);
void EWLClearTraceProfile(const void *inst);
i32 EWLGetVcmdCoreNum(const void *ctx, CLIENT_TYPE client_type);
void EWLReadVcmdPriority(const void *ctx, u32 *priority, u32 client_type);

char *EWLGetDevName(const void *inst);
char *EWLGetMemDevName(const void *inst);
i32 EWLGetConfigRegister(const void *inst, u32 core_id, u32 client_type, u32 offset);
void EWLSetUfbcMode(const void *inst, u32 is_enable, u32 hwId);
#ifdef __cplusplus
}
#endif
#endif /*__EWL_H__*/
