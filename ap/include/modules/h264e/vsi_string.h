/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2019 VeriSilicon Inc                       --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/
#ifndef _VSI_STRING_H_
#define _VSI_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif

#ifdef SAFESTRING
//safestring imp for Intel
#include "safe_lib.h"
#include "snprintf_s.h"

#define strlen(dest) strnlen_s(dest, 512)
//#define wcslen(dest)              wcsnlen_s(dest, 512)

#define strcat(dest, src) strcat_s(dest, 512, src)
//#define strncat(dest,src,n)       strncat_s(dest, 512, src, n)

#define strcpy(dest, src) strcpy_s(dest, 512, src)
#define strncpy(dest, src, n) strncpy_s(dest, 512, src, n)

#define memcpy(dest, src, n) memcpy_s(dest, n, src, n)
#define memmove(dest, src, n) memmove_s(dest, n, src, n)
/* #define memcmp(s1,s2,n)           int ind = 0;\
                                  int rc = 0;\
                                  if (rc = memcmp_s(s1, 512, s2, n, &ind) != EOK ) {\
    	                            printf("%s %u  Ind=%d  Error rc=%u \n", __FUNCTION__, __LINE__, ind, rc);\
									assert(0);\
                                  }\
	                                return ind; */

#define memset(dest, value, n) memset_s(dest, n, value)

typedef errno_t mem_ret;
av_unused static mem_ret vsi_strcmp(const char *s1, const char *s2) {
  int ind = 0;
  int rc = 0;
  if ((rc = strcmp_s(s1, RSIZE_MAX_STR, s2, &ind)) != EOK) {
    printf("%s %d  Ind=%d  Error rc=%d \n", __FUNCTION__, __LINE__, ind, rc);
    assert(0);
  }

  return ind;
}

#define strcmp vsi_strcmp
//...
#else
/* For memset, strcpy and strlen */
#include <string.h>

#define snprintf_s_i(dest, n, format, a) snprintf(dest, n, format, a)
#define snprintf_s_si(dest, dmax, format, s, a) \
  snprintf(dest, dmax, format, s, a)
#define snprintf_s_l(dest, dmax, format, a) snprintf(dest, dmax, format, a)
#define snprintf_s_sl(dest, dmax, format, s, a) \
  snprintf(dest, dmax, format, s, a)
#define snprintf_s_s(dest, dmax, format, s) snprintf(dest, dmax, format, s)
#define snprintf_s_sssiii(dest, dmax, format, s1, s2, s3, inta1, inta2, inta3) \
  snprintf(dest, dmax, format, s1, s2, s3, inta1, inta2, inta3)

typedef void *mem_ret;

av_unused static int vsi_strcmp(const char *s1, const char *s2) {
  return strcmp(s1, s2);
}

#endif /* SAFESTRING */

#ifdef __cplusplus
}
#endif

#endif /* _VSI_STRING_H_ */
