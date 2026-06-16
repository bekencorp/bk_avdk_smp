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

#include "common.h"
#include <components/bk_platform.h>
#include <driver/aon_rtc.h>

#include <string.h>

#define RNG_TEST_BUF_SIZE      32
#define RNG_TEST_SAMPLE_CNT    16

static int buffer_is_all_same(const uint8_t *buf, size_t len, uint8_t value)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i] != value) {
			return 0;
		}
	}
	return 1;
}

int te200_rand_basic_test(void)
{
	uint8_t buf[RNG_TEST_BUF_SIZE] = {0};
	int ret;

	ret = bk_fill_rand(NULL, RNG_TEST_BUF_SIZE);
	if (ret != -1) {
		BK_LOGE(NULL, "bk_fill_rand(NULL) expect -1, got %d\r\n", ret);
		return 1;
	}

	ret = bk_fill_rand(buf, 0);
	if (ret != -1) {
		BK_LOGE(NULL, "bk_fill_rand(len=0) expect -1, got %d\r\n", ret);
		return 1;
	}

	ret = bk_fill_rand(buf, sizeof(buf));
	if (ret != 0) {
		BK_LOGE(NULL, "bk_fill_rand failed: %d\r\n", ret);
		return 1;
	}

	if (buffer_is_all_same(buf, sizeof(buf), 0)) {
		BK_LOGE(NULL, "bk_fill_rand output all zero\r\n");
		return 1;
	}

	return 0;
}

int te200_rand_uniqueness_test(void)
{
	int samples[RNG_TEST_SAMPLE_CNT];
	uint8_t buf1[RNG_TEST_BUF_SIZE];
	uint8_t buf2[RNG_TEST_BUF_SIZE];
	int same_cnt = 0;

	for (int i = 0; i < RNG_TEST_SAMPLE_CNT; i++) {
		samples[i] = bk_rand();
	}

	for (int i = 1; i < RNG_TEST_SAMPLE_CNT; i++) {
		if (samples[i] == samples[0]) {
			same_cnt++;
		}
	}

	if (same_cnt == RNG_TEST_SAMPLE_CNT - 1) {
		BK_LOGE(NULL, "bk_rand samples are identical\r\n");
		return 1;
	}

	if (bk_fill_rand(buf1, sizeof(buf1)) != 0 ||
	    bk_fill_rand(buf2, sizeof(buf2)) != 0) {
		BK_LOGE(NULL, "bk_fill_rand failed in uniqueness test\r\n");
		return 1;
	}

	if (memcmp(buf1, buf2, sizeof(buf1)) == 0) {
		BK_LOGE(NULL, "bk_fill_rand outputs are identical\r\n");
		return 1;
	}

	return 0;
}

int te200_rand_loop_test(uint32_t test_cnt)
{
	uint8_t buf[RNG_TEST_BUF_SIZE];
	unsigned long long start_tick, end_tick, tick_cnt;

	if (test_cnt == 0) {
		test_cnt = 1000;
	}

	start_tick = bk_aon_rtc_get_us();
	for (uint32_t i = 0; i < test_cnt; i++) {
		if (bk_fill_rand(buf, sizeof(buf)) != 0) {
			BK_LOGE(NULL, "bk_fill_rand loop failed at %u\r\n", i);
			return 1;
		}
	}
	end_tick = bk_aon_rtc_get_us();
	tick_cnt = end_tick - start_tick;

	BK_DUMP_OUT("[rand]: run %u times, len %u bytes, cost %llu us, perf %.2f KB/s\r\n",
	            test_cnt, (unsigned)sizeof(buf), tick_cnt,
	            tick_cnt ? (1.0 * sizeof(buf) * test_cnt) / tick_cnt : 0.0);

	return 0;
}
