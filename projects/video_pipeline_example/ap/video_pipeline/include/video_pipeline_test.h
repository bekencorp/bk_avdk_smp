// Copyright 2020-2021 Beken
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

#pragma once

#include "components/bk_video_pipeline/bk_video_pipeline.h"
#include "jpeg_data.h"
#include "cli.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"
/**
 * @brief CLI command for video pipeline test
 *
 * @param cmd Command name
 * @param argc Number of arguments
 * @param argv Arguments list
 * @param user_data User data
 */
void cli_video_pipeline_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

/**
 * @brief CLI command for regular video pipeline test
 *
 * @param cmd Command name
 * @param argc Number of arguments
 * @param argv Arguments list
 * @param user_data User data
 */
void cli_video_pipeline_regular_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

/**
 * @brief CLI command for error video pipeline test
 *
 * @param argc Number of arguments
 * @param argv Arguments list
 * @return 0 on success, negative on failure
 */
void cli_video_pipeline_error_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#ifdef __cplusplus
}
#endif