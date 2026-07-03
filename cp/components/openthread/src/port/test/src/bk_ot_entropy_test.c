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

#include <string.h>
#include <stdlib.h>
#include <os/os.h>
#include <os/mem.h>

#include "bk_cli.h"

#include <openthread/platform/entropy.h>
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define BK_OT_ENTROPY_TST_LOG_EN 1
#if BK_OT_ENTROPY_TST_LOG_EN
#define bk_ot_entropy_log os_printf
#else
#define bk_ot_entropy_log
#endif

static void cli_ot_entropy_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if(argc<2)
    {
        bk_ot_entropy_log("%s please enter more than 2 parameters\r\n");
        return;
    }
    if(strncmp(argv[1],"get",sizeof("get"))==0)
    {
        if(argc == 2)
        {
            uint32_t seed=0;
            otPlatEntropyGet((uint8_t*)&seed,sizeof(seed));
            bk_ot_entropy_log("const seed:0x%x\r\n",seed);
        }
        else
        {
            uint16_t ulen=strtoul(argv[2],NULL,0);
            uint8_t *uSeed=psram_malloc(ulen);
            otPlatEntropyGet(uSeed,ulen);
            for(uint16_t i=0;i<ulen;i++)
            {
                bk_ot_entropy_log("[%d]=0x%x ",i,uSeed[i]);
            }
            psram_free(uSeed);
        }
    }
}
static int hdlMbedtlsEntropyPoll(void *aData, unsigned char *aOutput, size_t aInLen, size_t *aOutLen)
{
    int rval = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    rval=otPlatEntropyGet(aOutput,aInLen);
    if(rval != 0)
        rval= MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
    *aOutLen = aInLen;
    return rval;
}
static void cli_ot_entropy_selftest_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret=1;
    mbedtls_entropy_context ctx;
    mbedtls_ctr_drbg_context drbgCtx;
    mbedtls_entropy_init(&ctx);
    mbedtls_entropy_add_source(&ctx,hdlMbedtlsEntropyPoll,NULL,10,MBEDTLS_ENTROPY_SOURCE_STRONG);
    mbedtls_ctr_drbg_init(&drbgCtx);
    ret = mbedtls_ctr_drbg_seed(&drbgCtx,mbedtls_entropy_func,&ctx,NULL,0);
    if(ret !=0 )
        bk_ot_entropy_log("[Error] entropy to seed err:0x%x\r\n",ret);
    else
        bk_ot_entropy_log("[Success] entropy is active\r\n");

    mbedtls_entropy_free(&ctx);
    mbedtls_ctr_drbg_free(&drbgCtx);
}
#define OT_ENTROPY_CMD_CNT (sizeof(s_ot_entropy_commands)/sizeof(struct cli_command))
static const struct cli_command s_ot_entropy_commands[] = {
    {"otEntropy", "otEntropy get", cli_ot_entropy_cmd},
    {"otEntropySelfTest", "openthread entropy interface selftest", cli_ot_entropy_selftest_cmd},
};
int cli_ot_entropy_test_init(void)
{
    return cli_register_commands(s_ot_entropy_commands, OT_ENTROPY_CMD_CNT);
}
