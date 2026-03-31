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


#ifndef BQUEUE_H_DEFINED
#define BQUEUE_H_DEFINED

#include "basetype.h"
#include "dwlthread.h"

typedef struct
{
    u32 picI[16];
    u32 ctr;
    u32 queueSize;
    u32 prevAnchorSlot;
#ifdef USE_OUTPUT_RELEASE
    u32 bufUsed[16];
    u32 abort;
    pthread_mutex_t buf_release_mutex;
    pthread_cond_t buf_release_cv;
#endif
} bufferQueue_t;

#define BQUEUE_UNUSED (u32)(0xffffffff)

u32  BqueueInit( bufferQueue_t *bq, u32 numBuffers );
void BqueueRelease( bufferQueue_t *bq );
u32  BqueueNext( bufferQueue_t *bq, u32 ref0, u32 ref1, u32 ref2, u32 bPic );
void BqueueDiscard( bufferQueue_t *bq, u32 buffer );
#ifdef USE_OUTPUT_RELEASE
u32  BqueueInit2( bufferQueue_t *bq, u32 numBuffers );
void BqueueRelease2( bufferQueue_t *bq );
u32  BqueueNext2( bufferQueue_t *bq, u32 ref0, u32 ref1, u32 ref2, u32 bPic );
void BqueuePictureRelease( bufferQueue_t *bq, u32 buffer );
u32 BqueueWaitNotInUse( bufferQueue_t *bq);
void BqueueSetBufferAsUsed(bufferQueue_t *bq, u32 buffer);
u32 BqueueWaitBufNotInUse(bufferQueue_t *bq, u32 buffer);
void BqueueSetAbort(bufferQueue_t *bq);
void BqueueClearAbort(bufferQueue_t *bq);
#ifdef USE_EXTERNAL_BUFFER
void BqueueEmpty(bufferQueue_t *bq);
#endif
#endif

#endif /* BQUEUE_H_DEFINED */
