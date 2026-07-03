// Copyright 2020-2025 Beken
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

#include <stdio.h>
#include <string.h>
#include <openthread/error.h>
#include <openthread/platform/entropy.h>
// #include "mbedtls/entropy.h"
#include "driver/trng.h"
#include "sdkconfig.h"

#if CONFIG_OPENTHREAD

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    if(aOutput == NULL)
        return OT_ERROR_FAILED;
    uint16_t uLen=aOutputLength;
    uint32_t seed=0;
    uint8_t *uTmpBuf=(uint8_t *)aOutput;
    uint32_t uActSize=0;
    while(uLen>0)
    {
        seed = bk_rand();
        uActSize = MIN(sizeof(seed),uLen);
        memcpy(uTmpBuf,&seed,uActSize);
        uTmpBuf +=uActSize;
        uLen -=uActSize;
    }
    return OT_ERROR_NONE;
}
// otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
// {
//     int ret=1;
//     mbedtls_entropy_context ctx;
//     mbedtls_entropy_init(&ctx);
//     if((ret = mbedtls_entropy_gather(&ctx))!=0)
//         goto cleanup;
//     if((ret = mbedtls_entropy_update_manual(&ctx,aOutput,aOutputLength))!=0)
//         goto cleanup;

//     ret=mbedtls_entropy_func(&ctx,aOutput,aOutputLength);

// cleanup:
//     mbedtls_entropy_free(&ctx);
//     if(ret == 0)
//     return OT_ERROR_NONE;
//     else
//     return OT_ERROR_FAILED;
// }


#endif // CONFIG_OPENTHREAD