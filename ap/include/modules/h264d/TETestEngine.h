/*------------------------------------------------------------------------------
--         Copyright (c) 2015, VeriSilicon Inc. All rights reserved.          --
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

#ifndef __TETESTENGINE_H__
#define __TETESTENGINE_H__

#include "basetype.h"
#include <stdio.h>

#define malloc(a)                    pvPortMalloc(a)
#define realloc(a,b)                 pvPortRealloc(a,b)
#define free(a)                      vPortFree(a)

typedef enum {
    TE_OK = 0,
    TE_CANNOT_OPEN_LOG_FILE,
    TE_CANNOT_OPEN_RESULTS_FILE,
    TE_INVALID_CMD_LINE_PARAMETERS,
    TE_ALREADY_INITIALIZED,
    TE_NOT_INITIALIZED,
    TE_OUT_OF_MEMORY,
    TE_UNKNOWN_TEST_SUITE,
    TE_TEST_SUITE_ALREADY_ADDED,
    TE_UNKNOWN_TEST,
    TE_TEST_ALREADY_ADDED,
    TE_INVALID_TEST_ID,
    TE_CHECK_FAIL,
    TE_TEST_ID_MISSING
} TEError;

#define TE_INITIALIZE(argc, pArgv) \
TEError error = TE_OK;\
\
error = TEInitialize(argc, pArgv);\
if (TE_OK != error) { \
    TEPrintError(error);\
    TEFinalize();\
    return 1;\
}

#define TE_ADD_TEST_SUITE(pTestSuiteName, testSuiteId)\
error = TEAddTestSuite(pTestSuiteName, #pTestSuiteName, testSuiteId);\
if (TE_OK != error) { \
    TEPrintError(error);\
    TEFinalize();\
    return 1;\
}

#define TE_ADD_TEST(pTestName)\
error = TEAddTest(pTestName, #pTestName, testSuiteId, SetUp, TearDown);\
if (TE_OK != error) { \
    return error;\
}

#define TE_FINALIZE \
error = TERun(); \
if (TE_OK != error && TE_CHECK_FAIL != error) { \
    TEPrintError(error);\
}\
TEFinalize();

#define CHECK(expression) \
error = TECheck(expression, #expression, __FILE__, __LINE__);\
if (TE_OK != error) { \
    TEPrintError(error);\
    return error;\
}

#define TE_INCREMENT_TEST_ID(newTestId) \
error = TEIncrementTestId(newTestId);\
if (TE_OK != error) { \
    TEPrintError(error);\
    return error;\
}

typedef struct {
    FILE* pLogFile;
    FILE* pResultsFile;
    u32 testCaseId;
#if 0
    u32 testSuiteId;
    char* pTestCaseList;
#endif
} TEConfiguration;

typedef TEError (*TETestSuiteInit)(u32 testSuiteId);

typedef TEError (*TETestFunction)(void);

typedef TEError (*TETestSetUp)(void);

typedef TEError (*TETestTearDown)(void);

extern char pTEErrorReason[1000];

extern TEConfiguration* pTEConfiguration;

extern TEError TEInitialize(int argc, char** pArgv);

extern TEError TEAddTestSuite(TETestSuiteInit testSuiteInit, const char* pTestSuiteName, u32 testSuiteId);

extern TEError TEAddTest(TETestFunction testFunction, const char* pTestName, u32 testSuiteId, TETestSetUp setUp, TETestTearDown tearDown);

extern TEError TERun(void);

extern void TEFinalize(void);

extern void TEPrintError(TEError error);

extern void TELog(const char* pText);

extern TEError TECheck(i32 expression, const char* pExpressionStr, const char* pFile, u32 line);

extern TEError TEIncrementTestId(u32 newTestId);

extern u32 TEGetCurrentTestId(void);

#endif /* __TETESTENGINE_H__ */
