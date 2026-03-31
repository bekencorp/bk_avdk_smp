#pragma once

#include "hspl/hspl_res_lock.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_sspl_res_lock(bk_hspl_res_t res);
bk_err_t bk_sspl_res_unlock(bk_hspl_res_t res);

#ifdef __cplusplus
}
#endif
