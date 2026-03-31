/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2015 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------

--
--  Abstract : header file of Encoder Wrapper Layer Common Part
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#ifndef __EWL_MEMSYNC_H__
#define __EWL_MEMSYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"
#include "ewl.h"

/*If want to define MEM_SYNC_TEST, the premise is open SUPPORT_MEM_SYNC, otherwise, it doesn't make sense*/
#ifdef SUPPORT_MEM_SYNC
//#define MEM_SYNC_TEST
#else
#ifdef MEM_SYNC_TEST
#undef MEM_SYNC_TEST
#endif
#endif

/**
 * \addtogroup ewl_mem
 *
 * @{
 */
/** \brief The address information of the buffer for test verification of memory synchronization.
 * \n <b>NOTE</b>: This structure can be overwritten. */
struct sync_mem {
  /** \brief The aligned virtual address of the buffer from the host view. */
  u32 *dev_va;
  /** \brief The aligned physical address of the buffer from the host view. */
  ptr_t dev_pa_alloc;
  /** \brief The original virtual address of the buffer in device from the host view. */
  u32 *dev_va_alloc;
};

/** Transfers data between the host and device.
 *
 * This function is available only if memory synchronization is supported.
 *
 * \param [in] mem The buffer information used for synchronization.
 * \param [in] offset The offset related to the hardware register base address, in bytes.
 * \param [in] length The number of bytes to be synchronized.
 * \param [in] dir The direction of transferring data.
 * \return <tt>EWL_OK</tt>
* \return <tt>EWL_PAR_ERROR</tt>
 */
i32 EWLSyncMemData(EWLLinearMem_t *mem, u32 offset, u32 length,
                   enum EWLMemSyncDirection dir);

/** Allocates memory for a buffer in the host.
 *
 * This function is available only if memory synchronization is supported.
 *
 * \param [in] alignment The alignment for allocation.
 * \param [in] size The bytes to be allocated.
 * \param [out] buff The information of the memory allocated as a buffer in the host.
 * \return <tt>EWL_OK</tt>
 */
i32 EWLMemSyncAllocHostBuffer(const void *inst, u32 size, u32 alignment,
                              EWLLinearMem_t *buff);

/** Releases the memory of a buffer in the host.
 *
 * This function is available only if memory synchronization is supported.
 *
 * \param [in] buff  The information of the memory to be released.
 * \return <tt>EWL_OK</tt>
 */
i32 EWLMemSyncFreeHostBuffer(const void *inst, EWLLinearMem_t *buff);

/**@}*/

#ifndef SUPPORT_MEM_SYNC
#define EWLSyncMemData(mem, offset, length, dir) (EWL_OK)
#define EWLMemSyncAllocHostBuffer(inst, size, alignment, buff) (EWL_NOT_SUPPORT)
#define EWLMemSyncFreeHostBuffer(inst, buff) (EWL_NOT_SUPPORT)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __EWL_MEMSYNC_H__ */
