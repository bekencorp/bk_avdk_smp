/**
 * @file lv_baf.h
 */

#ifndef LV_BAF_H
#define LV_BAF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../lv_conf_internal.h"
#include "../../misc/lv_types.h"
#include "../../core/lv_obj_class.h"
#include "../../widgets/image/lv_image.h"
#include <bk_baf_types.h>
#include LV_STDBOOL_INCLUDE
#include LV_STDINT_INCLUDE

#if LV_USE_BAF

typedef struct _lv_baf_t lv_baf_t;

LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_baf_class;

lv_obj_t * lv_baf_create(lv_obj_t * parent);
void lv_baf_set_src(lv_obj_t * obj, const bk_baf_source_t * src);

void lv_baf_restart(lv_obj_t * obj);
void lv_baf_pause(lv_obj_t * obj);
void lv_baf_resume(lv_obj_t * obj);

bool lv_baf_is_loaded(lv_obj_t * obj);
int32_t lv_baf_get_loop_count(lv_obj_t * obj);
void lv_baf_set_loop_count(lv_obj_t * obj, int32_t count);

#endif /* LV_USE_BAF */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_BAF_H */
