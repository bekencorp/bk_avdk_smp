// Dual-interpreter (person + pet) parallel inference demo.
//
// Design (see TFLM_多解释器多线程并行_设计文档.md):
//  - Two different models run in two core-pinned tasks: person detection on
//    AP core0 (CPU2) and pet (YOLOv8n) detection on AP core1 (CPU3).
//  - All initialization happens in the single-threaded phase (main) to rule out
//    the Register_ETHOSU static-registration race and lazy-init races.
//  - Per-instance resources: model copy, tensor arena, op resolver, interpreter
//    and postprocess buffers. Shared (single copy): the Ethos-U NPU driver and
//    its fast scratch, serialized by the driver reserve/release mechanism.
//  - __dso_handle is NOT redefined here: the single definition in
//    tflm_person_detection_demo.cpp satisfies the whole link.

// C RTOS/SDK headers FIRST, with C linkage. os/os.h has no internal
// extern "C" guards and this TU calls its APIs directly (rtos_core*_create_
// psram_thread / rtos_delay_milliseconds / rtos_get_core_id), so its first
// inclusion must be inside extern "C" (unlike the person/pet demos, which
// call these APIs from ap_main.c and can rely on C TU linkage).
extern "C" {
#include "os/os.h"
#include "os/str.h"
#include <components/system.h>
#include <components/bk_frame_buffer.h>
#include "dwt.h"
}

#include "tflm_multi_model_demo.h"
#include "tflm_pet_postprocess.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <new>

#include "components/log.h"
#include "os/mem.h"

// Person detection model + input provider (existing, read-only reuse)
#include "tflm_person_detection_model_data.h"
#include "tflm_person_detection_model_settings.h"
#include "tflm_person_detection_image_provider.h"

#include "bk_ethosu.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/kernels/conv.h"

static char TAG[] = "tflm_multi";
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

#define TFLM_INTERPRETER_NUM           2

// Shared NPU fast scratch: one buffer serving both models (driver-mutexed),
// sized by the larger vela arena_cache_size requirement.
#define TFLM_MULTI_ETHOSU_SCRATCH_SIZE (250 * 1024)   // max(person 250KB, pet 130KB)

// Person model arena (same as the original single-interpreter demo).
#define TFLM_PERSON_ARENA_SIZE         (100 * 1024)

// Real-scenario periods: person 10fps-class, pet 2fps-class (YOLOv8n is heavy).
#define TFLM_PERSON_PERIOD_MS          100
#define TFLM_PET_PERIOD_MS             500

// Print invoke time every N rounds (1 = every round).
#define TFLM_MULTI_LOG_INTERVAL        1

#define TFLM_MULTI_TASK_PRIORITY       (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
#define TFLM_MULTI_TASK_STACK_SIZE     (1024 * 8)

// ---------------------------------------------------------------------------
// Memory policies (per design doc: person -> PSRAM heap, pet -> MEM_SLAB_UNCODED)
// ---------------------------------------------------------------------------

typedef enum {
    TFLM_MEM_PSRAM = 0,   // psram_malloc (AP_PSRAM_HEAP region)
    TFLM_MEM_UNCODED,     // bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED)
} tflm_mem_policy_t;

static const char *tflm_mem_policy_name(tflm_mem_policy_t policy)
{
    return (policy == TFLM_MEM_PSRAM) ? "PSRAM" : "MEM_SLAB_UNCODED";
}

static void *tflm_policy_alloc(tflm_mem_policy_t policy, size_t size)
{
    if (policy == TFLM_MEM_PSRAM) {
        return psram_malloc(size);
    }
    if (size == 0U || size > (size_t)UINT32_MAX) {
        return nullptr;
    }
    return bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, (uint32_t)size);
}

static void tflm_policy_free(void *ptr, tflm_mem_policy_t policy)
{
    if (ptr == nullptr) {
        return;
    }
    if (policy == TFLM_MEM_PSRAM) {
        psram_free(ptr);
    } else {
        bk_frame_buffer_free(ptr);
    }
}

// ---------------------------------------------------------------------------
// Per-instance context
// ---------------------------------------------------------------------------

struct tflm_infer_ctx;

struct tflm_model_desc {
    const char *name;                                  // "person" / "pet"
    const unsigned char *model_data;                   // flash vela tflite array
    size_t model_data_len;
    size_t arena_size;
    tflm_mem_policy_t mem_policy;
    bk_err_t (*prepare_input)(struct tflm_infer_ctx *ctx);
    bk_err_t (*process_output)(struct tflm_infer_ctx *ctx);
};

struct tflm_infer_ctx {
    const struct tflm_model_desc *desc;
    const char *tag;                     // log prefix: "T0" / "T1"
    uint32_t period_ms;                  // inference period (real-scenario mode)

    uint8_t *model_raw;                  // raw allocation (free handle)
    uint8_t *model_data;                 // 16-byte aligned model copy (NPU requirement)
    size_t model_data_size;
    uint8_t *tensor_arena;               // per-instance arena

    tflite::MicroMutableOpResolver<13> op_resolver;             // per-instance
    tflite::MicroInterpreter *interpreter;                      // points to storage below
    alignas(tflite::MicroInterpreter)
        uint8_t interpreter_storage[sizeof(tflite::MicroInterpreter)];

    struct pet_postproc_buf *pet_buf;     // pet instance only

    bool use_person_image;                // current person input (T0 only)

    uint32_t run_count;
    bool initialized;
};

// ---------------------------------------------------------------------------
// Model-specific input/output hooks
// ---------------------------------------------------------------------------

// --- person: 96x96x1 grayscale, single score output ---

static bk_err_t person_prepare_input(struct tflm_infer_ctx *ctx)
{
    TfLiteTensor *input = ctx->interpreter->input(0);
    if (input == nullptr) {
        LOGE("[%s] input tensor is null\r\n", ctx->tag);
        return BK_FAIL;
    }
    if (input->dims->size != 4 ||
            input->dims->data[1] != kNumRows ||
            input->dims->data[2] != kNumCols ||
            input->dims->data[3] != kNumChannels ||
            input->type != kTfLiteInt8) {
        LOGE("[%s] unexpected input properties\r\n", ctx->tag);
        return BK_FAIL;
    }

    // Alternate person / no-person images each round (both verified baseline results)
    // GetImage type semantics: 0 = person image, 1 = no-person image.
    ctx->use_person_image = ((ctx->run_count & 1U) == 0U);
    int8_t image_type = ctx->use_person_image ? 0 : 1;
    if (kTfLiteOk != GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8, image_type)) {
        LOGE("[%s] Image capture failed\r\n", ctx->tag);
        return BK_FAIL;
    }
    return BK_OK;
}

static bk_err_t person_process_output(struct tflm_infer_ctx *ctx)
{
    TfLiteTensor *output = ctx->interpreter->output(0);
    if (output == nullptr) {
        LOGE("[%s] output tensor is null\r\n", ctx->tag);
        return BK_FAIL;
    }

    // Same result path as the original single-interpreter person demo.
    int8_t person_score = output->data.uint8[kPersonIndex];
    float person_score_f = (person_score - output->params.zero_point) * output->params.scale;
    int person_score_int = (person_score_f) * 100 + 0.5;

    const char *image = ctx->use_person_image ? "person" : "no-person";
    LOGI("[%s:%s] input=%s person score:%d%%\r\n", ctx->tag, ctx->desc->name, image, person_score_int);
    return BK_OK;
}

// --- pet: 320x320x3 YOLOv8n, letterbox + dequant + NMS output ---

static bk_err_t pet_prepare_input(struct tflm_infer_ctx *ctx)
{
    TfLiteTensor *input = ctx->interpreter->input(0);
    if (input == nullptr) {
        LOGE("[%s] input tensor is null\r\n", ctx->tag);
        return BK_FAIL;
    }
    if (input->dims->size != 4 ||
            input->dims->data[1] != k_pet_input_h ||
            input->dims->data[2] != k_pet_input_w ||
            input->dims->data[3] != k_pet_input_c ||
            input->type != kTfLiteInt8) {
        LOGE("[%s] unexpected input properties\r\n", ctx->tag);
        return BK_FAIL;
    }

    // Alternate the embedded cat / dog inputs each round (both verified against
    // the pet_detection project's standalone results).
    const unsigned char *img;
    size_t img_len;
    if (ctx->run_count & 1U) {
        img = dog_204_model_input;
        img_len = dog_204_model_input_len;
    } else {
        img = cat_200_model_input;
        img_len = cat_200_model_input_len;
    }
    if ((size_t)input->bytes != img_len) {
        LOGE("[%s] input bytes %d != embedded %u\r\n", ctx->tag, (int)input->bytes, (unsigned)img_len);
        return BK_FAIL;
    }
    os_memcpy(input->data.int8, img, (size_t)input->bytes);
    return BK_OK;
}

static bk_err_t pet_process_output(struct tflm_infer_ctx *ctx)
{
    TfLiteTensor *output = ctx->interpreter->output(0);
    if (output == nullptr) {
        LOGE("[%s] output tensor is null\r\n", ctx->tag);
        return BK_FAIL;
    }
    if (output->type != kTfLiteInt8) {
        LOGE("[%s] expected int8 output\r\n", ctx->tag);
        return BK_FAIL;
    }

    int orig_w, orig_h;
    if (ctx->run_count & 1U) {
        orig_w = k_pet_dog_orig_w;
        orig_h = k_pet_dog_orig_h;
    } else {
        orig_w = k_pet_cat_orig_w;
        orig_h = k_pet_cat_orig_h;
    }

    float scale = 0.f;
    int pad_top = 0, pad_left = 0;
    pet_letterbox_params(orig_w, orig_h, &scale, &pad_top, &pad_left);

    // All NMS working state lives in ctx->pet_buf (per-instance).
    int nd = pet_postprocess_yolov8_int8(output->data.int8, output->params.scale,
                                         output->params.zero_point, scale, pad_top, pad_left,
                                         orig_w, orig_h, ctx->pet_buf);

    const char *image = (ctx->run_count & 1U) ? "dog_204" : "cat_200";
    LOGI("[%s:%s] input=%s (%dx%d) detections=%d\r\n", ctx->tag, ctx->desc->name, image, orig_w, orig_h, nd);
    for (int i = 0; i < nd; i++) {
        const struct PetDet *d = &ctx->pet_buf->dets[i];
        const char *nm = (d->class_id >= 0 && d->class_id < k_pet_num_classes)
                         ? k_pet_class_names[d->class_id] : "?";
        LOGI("  [%d] class_id=%d (%s) score=%.4f bbox_orig: x1=%.2f y1=%.2f x2=%.2f y2=%.2f\r\n",
             i, d->class_id, nm, d->score, d->x1, d->y1, d->x2, d->y2);
    }
    if (nd == 0) {
        LOGI("  (none above conf=%.2f)\r\n", k_pet_conf_threshold);
    }
    return BK_OK;
}

// ---------------------------------------------------------------------------
// Model descriptor table (all model differences converge here)
// ---------------------------------------------------------------------------

// NOTE: NOT const, and the model lengths are filled at runtime below.
// The _len fields are VALUES of extern const symbols defined in other TUs;
// using them in this static initializer cannot be resolved at link time and
// silently zeroed the fields in .data (this platform runs no C++ dynamic
// init: .init_array is empty). With len==0, memcpy() copied 0 bytes and the
// interpreter parsed PSRAM garbage -> MemFault. Pointer fields (model arrays,
// hooks) are plain address relocations and are safe in a static initializer.
static struct tflm_model_desc s_model_descs[TFLM_INTERPRETER_NUM] = {
    { "person", g_person_detection_model_vela_data,
      (size_t)g_person_detection_model_vela_data_len, (size_t)TFLM_PERSON_ARENA_SIZE,
      TFLM_MEM_PSRAM, person_prepare_input, person_process_output },
    { "pet", pet_detection_vela_tflite,
      (size_t)pet_detection_vela_tflite_len, k_pet_arena_size,
      TFLM_MEM_UNCODED, pet_prepare_input, pet_process_output },
};

static struct tflm_infer_ctx s_ctx[TFLM_INTERPRETER_NUM];
static void *s_ethosu_scratch_raw = nullptr;   // HSRAM allocation (free handle)

// ---------------------------------------------------------------------------
// Init / cleanup (single-threaded phase only)
// ---------------------------------------------------------------------------

static void tflm_multi_cleanup(void)
{
    for (int i = TFLM_INTERPRETER_NUM - 1; i >= 0; i--) {
        struct tflm_infer_ctx *ctx = &s_ctx[i];

        if (ctx->pet_buf != nullptr) {
            psram_free(ctx->pet_buf);
            ctx->pet_buf = nullptr;
        }
        if (ctx->tensor_arena != nullptr) {
            tflm_policy_free(ctx->tensor_arena, ctx->desc->mem_policy);
            ctx->tensor_arena = nullptr;
        }
        ctx->interpreter = nullptr;
        if (ctx->model_raw != nullptr) {
            tflm_policy_free(ctx->model_raw, ctx->desc->mem_policy);
            ctx->model_raw = nullptr;
            ctx->model_data = nullptr;
            ctx->model_data_size = 0;
        }
        ctx->initialized = false;
    }
    if (s_ethosu_scratch_raw != nullptr) {
        hsram_free(s_ethosu_scratch_raw);
        s_ethosu_scratch_raw = nullptr;
    }
    // NOTE: the NPU driver is intentionally left initialized (bk_ethosu_deinit
    // is a system-shutdown path, not part of demo failure recovery).
}

bk_err_t tflm_multi_demo_init(void)
{
    LOGI("===========>start multi-interpreter (person + pet) demo init\r\n");

    // Model data lengths must be read at RUNTIME (see NOTE at s_model_descs):
    // the values live in other TUs and the static initializer cannot capture
    // them on this platform (no C++ dynamic init at startup).
    s_model_descs[0].model_data_len = (size_t)g_person_detection_model_vela_data_len;
    s_model_descs[1].model_data_len = (size_t)pet_detection_vela_tflite_len;
    if (s_model_descs[0].model_data_len == 0U || s_model_descs[1].model_data_len == 0U) {
        LOGE("model data length resolved to 0 (person=%u, pet=%u)\r\n",
             (unsigned)s_model_descs[0].model_data_len, (unsigned)s_model_descs[1].model_data_len);
        return BK_FAIL;
    }

    // 1. Shared NPU fast scratch (single copy, driver-mutexed in use).
    s_ethosu_scratch_raw = hsram_malloc((size_t)TFLM_MULTI_ETHOSU_SCRATCH_SIZE + 16U);
    if (s_ethosu_scratch_raw == nullptr) {
        LOGE("alloc ethosu scratch failed, size=%d\r\n", TFLM_MULTI_ETHOSU_SCRATCH_SIZE);
        return BK_FAIL;
    }
    void *scratch_aligned = (void *)(((uint32_t)s_ethosu_scratch_raw + 15U) & ~15U);
    LOGI("ethosu scratch -> HSRAM %p size=%u\r\n", scratch_aligned, (unsigned)TFLM_MULTI_ETHOSU_SCRATCH_SIZE);

    // 2. Single global NPU init (idempotent-guarded in bk_ethosu.c, see R1).
    if (bk_ethosu_init(scratch_aligned, (uint32_t)TFLM_MULTI_ETHOSU_SCRATCH_SIZE) != BK_OK) {
        LOGE("bk_ethosu_init failed\r\n");
        hsram_free(s_ethosu_scratch_raw);
        s_ethosu_scratch_raw = nullptr;
        return BK_FAIL;
    }

    // 3. Per-instance init (both interpreters share the NPU driver above).
    for (int i = 0; i < TFLM_INTERPRETER_NUM; i++) {
        struct tflm_infer_ctx *ctx = &s_ctx[i];
        const struct tflm_model_desc *desc = &s_model_descs[i];

        ctx->desc = desc;
        ctx->tag = (i == 0) ? "T0" : "T1";
        ctx->period_ms = (i == 0) ? TFLM_PERSON_PERIOD_MS : TFLM_PET_PERIOD_MS;

        // Model copy: 16-byte aligned (Ethos-U command stream requirement).
        ctx->model_data_size = desc->model_data_len;
        ctx->model_raw = (uint8_t *)tflm_policy_alloc(desc->mem_policy, ctx->model_data_size + 16U);
        if (ctx->model_raw == nullptr) {
            LOGE("[%s] alloc model failed, size=%u\r\n", ctx->tag, (unsigned)(ctx->model_data_size + 16U));
            goto fail;
        }
        ctx->model_data = (uint8_t *)(((uintptr_t)ctx->model_raw + 15U) & ~(uintptr_t)15U);
        os_memcpy(ctx->model_data, desc->model_data, ctx->model_data_size);

        const tflite::Model *model = tflite::GetModel(ctx->model_data);
        if (model == nullptr) {
            LOGE("[%s] GetModel failed\r\n", ctx->tag);
            goto fail;
        }
        if (model->version() != TFLITE_SCHEMA_VERSION) {
            LOGE("[%s] model schema version mismatch: %d != %d\r\n", ctx->tag,
                 (int)model->version(), (int)TFLITE_SCHEMA_VERSION);
            goto fail;
        }

        // Tensor arena (per-instance).
        ctx->tensor_arena = (uint8_t *)tflm_policy_alloc(desc->mem_policy, desc->arena_size);
        if (ctx->tensor_arena == nullptr) {
            LOGE("[%s] alloc arena failed, size=%u\r\n", ctx->tag, (unsigned)desc->arena_size);
            goto fail;
        }

        // Pet postprocess working buffers (per-instance, multi-thread safe).
        if (i == 1) {
            ctx->pet_buf = (struct pet_postproc_buf *)psram_malloc(sizeof(struct pet_postproc_buf));
            if (ctx->pet_buf == nullptr) {
                LOGE("[%s] alloc pet postproc buf failed, size=%u\r\n", ctx->tag,
                     (unsigned)sizeof(struct pet_postproc_buf));
                goto fail;
            }
        }

        // Construct the resolver in place BEFORE first use: it has a vptr and a
        // non-trivial constructor, but static storage on this platform gets NO
        // construction run at startup (empty .init_array). Without this the
        // vptr stays NULL (bss) and the first virtual call (FindOp inside
        // AddCustom) faults with MemFault@0x4.
        new (&ctx->op_resolver) tflite::MicroMutableOpResolver<13>();

        // Op resolver registration: single-threaded phase, so the
        // Register_ETHOSU static-registration race cannot happen.
        if (ctx->op_resolver.AddEthosU() != kTfLiteOk) {
            LOGE("[%s] AddEthosU failed\r\n", ctx->tag);
            goto fail;
        }

        // Interpreter (placement new on per-instance storage).
        ctx->interpreter = new (ctx->interpreter_storage) tflite::MicroInterpreter(
                model, ctx->op_resolver, ctx->tensor_arena, desc->arena_size);

        if (ctx->interpreter->AllocateTensors() != kTfLiteOk) {
            LOGE("[%s] AllocateTensors failed (arena=%u bytes)\r\n", ctx->tag, (unsigned)desc->arena_size);
            goto fail;
        }

        ctx->initialized = true;
        LOGI("[%s:%s] ready: model %p (%u B, %s) arena %p (%u B, used %u, %s)\r\n",
             ctx->tag, desc->name, ctx->model_data, (unsigned)ctx->model_data_size,
             tflm_mem_policy_name(desc->mem_policy), ctx->tensor_arena, (unsigned)desc->arena_size,
             (unsigned)ctx->interpreter->arena_used_bytes(), tflm_mem_policy_name(desc->mem_policy));
    }

    LOGI("multi-interpreter init OK (shared NPU, %d interpreters)\r\n", TFLM_INTERPRETER_NUM);
    return BK_OK;

fail:
    tflm_multi_cleanup();
    return BK_FAIL;
}

// ---------------------------------------------------------------------------
// Inference tasks
// ---------------------------------------------------------------------------

static bk_err_t tflm_multi_run_one(struct tflm_infer_ctx *ctx)
{
    bk_err_t ret = ctx->desc->prepare_input(ctx);
    if (ret != BK_OK) {
        return ret;
    }

    // DWT cycle counter: per-core when tasks are core-pinned (T0 -> core0, T1 -> core1).
    dwt_init_cycle_counter();
    uint32_t start_cycles = dwt_get_cycle_counter_val();
    TfLiteStatus invoke_ret = ctx->interpreter->Invoke();
    uint32_t cycles = dwt_get_cycle_counter_val() - start_cycles;

    if (invoke_ret != kTfLiteOk) {
        LOGE("[%s:%s] Invoke failed\r\n", ctx->tag, ctx->desc->name);
        return BK_FAIL;
    }

    if ((ctx->run_count % TFLM_MULTI_LOG_INTERVAL) == 0U) {
        uint32_t us = cycles / 480U;    // 480 MHz AP clock
        if (us < 1000U) {
            LOGI("[%s:%s] Invoke time: %u us\r\n", ctx->tag, ctx->desc->name, us);
        } else {
            LOGI("[%s:%s] Invoke time: %u ms\r\n", ctx->tag, ctx->desc->name, us / 1000U);
        }
    }

    ret = ctx->desc->process_output(ctx);
    ctx->run_count++;
    return ret;
}

static void tflm_multi_task_entry(void *arg)
{
    struct tflm_infer_ctx *ctx = (struct tflm_infer_ctx *)arg;

    LOGI("[%s:%s] started on core %d (period %u ms)\r\n", ctx->tag, ctx->desc->name,
         (int)(rtos_get_core_id() & 1U), (unsigned)ctx->period_ms);

    while (1) {
        (void)tflm_multi_run_one(ctx);
        rtos_delay_milliseconds(ctx->period_ms);
    }

    rtos_delete_thread(NULL);
}

// ---------------------------------------------------------------------------
// Task creation
// ---------------------------------------------------------------------------

bk_err_t tflm_multi_demo_start(void)
{
    bk_err_t ret;
    static beken_thread_t s_task_person = NULL;
    static beken_thread_t s_task_pet = NULL;

    ret = rtos_core0_create_psram_thread(&s_task_person, TFLM_MULTI_TASK_PRIORITY, "tflm_person",
                                         tflm_multi_task_entry, TFLM_MULTI_TASK_STACK_SIZE, &s_ctx[0]);
    if (ret != BK_OK) {
        LOGE("create tflm_person task failed, ret=%d\r\n", ret);
        return ret;
    }

    ret = rtos_core1_create_psram_thread(&s_task_pet, TFLM_MULTI_TASK_PRIORITY, "tflm_pet",
                                        tflm_multi_task_entry, TFLM_MULTI_TASK_STACK_SIZE, &s_ctx[1]);
    if (ret != BK_OK) {
        LOGE("create tflm_pet task failed, ret=%d\r\n", ret);
        return ret;
    }

    LOGI("dual-interpreter tasks started (core0: person, core1: pet)\r\n");
    return BK_OK;
}
