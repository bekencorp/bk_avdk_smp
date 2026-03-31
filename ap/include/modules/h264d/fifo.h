/*------------------------------------------------------------------------------
--         Copyright (c) 2015, VeriSilicon Inc. All rights reserved           --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
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
------------------------------------------------------------------------------*/


#ifndef __FIFO_H__
#define __FIFO_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#define malloc(a)                    pvPortMalloc(a)
#define realloc(a,b)                 pvPortRealloc(a,b)
#define calloc(a,b)                  pvPortCalloc(a,b)
#define free(a)                      vPortFree(a)

/* FIFO_DATATYPE must be defined to hold specific type of pointers. If it is not
 * defined, by default the datatype will be void. */
#ifndef FIFO_DATATYPE
#error "You must define FIFO_DATATYPE to use this module."
#endif  /* FIFO_DATATYPE */

typedef enum
{
    FIFO_OK,
    FIFO_ERROR_MEMALLOC,
    FIFO_ABORT = 0xFFFF
} fifo_ret;

typedef void* fifo_inst;
typedef FIFO_DATATYPE fifo_object;

/* fifo_init initializes the queue.
 * |num_of_slots| defines how many slots to reserve at maximum.
 * |pInstance| is output parameter holding the instance. */
fifo_ret fifo_init(u32 num_of_slots, fifo_inst* instance);

/* fifo_push pushes an object to the back of the queue. Ownership of the
 * contained object will be moved from the caller to the queue. Returns number
 * of elements in the queue after the push.
 *
 * |inst| is the instance push to.
 * |object| holds the pointer to the object to push into queue. */
u32 fifo_push(fifo_inst inst, fifo_object object);

/* fifo_pop returns object from the front of the queue. Ownership of the popped
 * object will be moved from the queue to the caller. Returns number of elements
 * in the queue after the pop.
 *
 * |inst| is the instance to pop from.
 * |object| holds the pointer to the object popped from the queue. */
u32 fifo_pop(fifo_inst inst, fifo_object* object);

/* Ask how many objects there are in the fifo. */
u32 fifo_count(fifo_inst inst);

/* fifo_release releases and deallocated queue. User needs to make sure the
 * queue is empty and no threads are waiting in fifo_push or fifo_pop.
 * |inst| is the instance to release. */
void fifo_release(fifo_inst inst);

void fifo_set_abort(fifo_inst inst);

void fifo_clear_abort(fifo_inst inst);
#endif  /* __FIFO_H__ */

