#include "tflm_person_detection_demo.h"

#include <string.h>
#include <stdlib.h>

#include "components/log.h"
#include "os/mem.h"

// Use C linkage for RTOS APIs and __dso_handle to avoid C++ name mangling issues.
extern "C" {
#include "os/os.h"
#include <components/system.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
void * __dso_handle = 0;
#include "dwt.h"
}

#include "tflm_person_detection_model_data.h"
#include "tflm_person_detection_model_settings.h"
#include "tflm_person_detection_image_provider.h"
#include "ethosu_driver.h"
#include "bk_ethosu.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/kernels/conv.h"

static char TAG[] = "tflm_gd";
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)



//#define PERSON_DETECTION_DEBUG   //GPIO debug
// Set to 1 to enable performance mode:
// Prefer HSRAM first, then fallback to SRAM(os_malloc) when HSRAM is not enough.
#define PERSON_DETECTION_PERF_TEST 0

// Set to 1 to enable inference time statistics using DWT.
// Time is computed with fixed CPU frequency 480MHz.
#define CONFIG_PERSON_DETECTION_INFER_TIME_AUTO

#if PERSON_DETECTION_PERF_TEST
typedef enum {
    TFLM_MEM_SRC_NONE = 0,
    TFLM_MEM_SRC_HSRAM,
    TFLM_MEM_SRC_SRAM
} tflm_mem_src_t;

static const char *tflm_mem_src_name(tflm_mem_src_t src)
{
    if (src == TFLM_MEM_SRC_HSRAM) {
        return "HSRAM";
    }
    if (src == TFLM_MEM_SRC_SRAM) {
        return "SRAM(os_malloc)";
    }
    return "NONE";
}

static void *tflm_runtime_alloc(size_t size, tflm_mem_src_t *src)
{
    void *ptr = hsram_malloc(size);
    if (ptr != nullptr) {
        if (src != nullptr) {
            *src = TFLM_MEM_SRC_HSRAM;
        }
        return ptr;
    }

    ptr = os_malloc(size);
    if (ptr != nullptr && src != nullptr) {
        *src = TFLM_MEM_SRC_SRAM;
    }
    return ptr;
}

static void tflm_runtime_free(void *ptr, tflm_mem_src_t src)
{
    if (ptr == nullptr) {
        return;
    }

    if (src == TFLM_MEM_SRC_HSRAM) {
        hsram_free(ptr);
    } else {
        os_free(ptr);
    }
}

static void *tflm_scratch_alloc(size_t size, tflm_mem_src_t *src)
{
    return tflm_runtime_alloc(size, src);
}

static void tflm_scratch_free(void *ptr, tflm_mem_src_t src)
{
    tflm_runtime_free(ptr, src);
}
#else
typedef enum {
    TFLM_MEM_SRC_NONE = 0,
    TFLM_MEM_SRC_HSRAM,
    TFLM_MEM_SRC_PSRAM
} tflm_mem_src_t;

static const char *tflm_mem_src_name(tflm_mem_src_t src)
{
    if (src == TFLM_MEM_SRC_HSRAM) {
        return "HSRAM";
    }
    return "PSRAM";
}

static void *tflm_runtime_alloc(size_t size, tflm_mem_src_t *src)
{
    void *ptr = psram_malloc(size);
    if (ptr != nullptr && src != nullptr) {
        *src = TFLM_MEM_SRC_PSRAM;
    }
    return ptr;
}

static void tflm_runtime_free(void *ptr, tflm_mem_src_t src)
{
    (void)src;
    psram_free(ptr);
}

static void *tflm_scratch_alloc(size_t size, tflm_mem_src_t *src)
{
    void *ptr = hsram_malloc(size);
    if (ptr != nullptr && src != nullptr) {
        *src = TFLM_MEM_SRC_HSRAM;
    }
    return ptr;
}

static void tflm_scratch_free(void *ptr, tflm_mem_src_t src)
{
    (void)src;
    hsram_free(ptr);
}
#endif

#ifdef PERSON_DETECTION_DEBUG

#define PERSON_DETECTION_DEBUG_GPIO_INIT(id)  do { gpio_dev_unmap(id); bk_gpio_enable_output(id); bk_gpio_set_output_low(id);} while (0)

#define PERSON_DETECTION_DEBUG_INIT()  do { PERSON_DETECTION_DEBUG_GPIO_INIT(GPIO_32); PERSON_DETECTION_DEBUG_GPIO_INIT(GPIO_33); PERSON_DETECTION_DEBUG_GPIO_INIT(GPIO_34); PERSON_DETECTION_DEBUG_GPIO_INIT(GPIO_35); } while (0)

#define PERSON_DETECTION_START()                            do { bk_gpio_set_output_low(GPIO_32); bk_gpio_set_output_high(GPIO_32);} while (0)
#define PERSON_DETECTION_END()                              do { bk_gpio_set_output_low(GPIO_32); } while (0)

#define PERSON_DETECTION_GET_IMAGE_START()                  do { bk_gpio_set_output_low(GPIO_33); bk_gpio_set_output_high(GPIO_33);} while (0)
#define PERSON_DETECTION_GET_IMAGE_END()                    do { bk_gpio_set_output_low(GPIO_33); } while (0)

#define PERSON_DETECTION_INFERENCE_START()                  do { bk_gpio_set_output_low(GPIO_34); bk_gpio_set_output_high(GPIO_34);} while (0)
#define PERSON_DETECTION_INFERENCE_END()                    do { bk_gpio_set_output_low(GPIO_34); } while (0)

#define PERSON_DETECTION_RESULT_PROCESS_START()             do { bk_gpio_set_output_low(GPIO_35); bk_gpio_set_output_high(GPIO_35);} while (0)
#define PERSON_DETECTION_RESULT_PROCESS_END()               do { bk_gpio_set_output_low(GPIO_35); } while (0)

#else

#define PERSON_DETECTION_DEBUG_INIT()

#define PERSON_DETECTION_START()
#define PERSON_DETECTION_END()

#define PERSON_DETECTION_GET_IMAGE_START()
#define PERSON_DETECTION_GET_IMAGE_END()

#define PERSON_DETECTION_INFERENCE_START()
#define PERSON_DETECTION_INFERENCE_END()

#define PERSON_DETECTION_RESULT_PROCESS_START()
#define PERSON_DETECTION_RESULT_PROCESS_END()

#endif


// Tensor arena size for person detection model.
#define TFLM_ARENA_SIZE (100 * 1024)
#define ETHOSU0_SCRATCH_SIZE (250 * 1024)

static struct ethosu_driver ethosu0_driver;


void bk_npu_int_isr(void)
{
    ethosu_irq_handler(&ethosu0_driver);
}

// Static interpreter pointer and PSRAM arena pointer (reused across multiple inferences)
// Tensor arena memory pointer
static uint8_t *g_tensor_arena = nullptr;
static tflite::MicroInterpreter *g_interpreter = nullptr;
static bool g_interpreter_initialized = false;
static void *g_ethosu0_scratch = nullptr;
static tflm_mem_src_t g_ethosu0_scratch_src = TFLM_MEM_SRC_NONE;

// Model data buffer for dynamic loading in PSRAM.
// Raw pointer for memory deallocation (original malloc address)
static uint8_t *g_model_data_raw = nullptr;
static tflm_mem_src_t g_model_data_src = TFLM_MEM_SRC_NONE;
// Aligned pointer for actual model data usage (16-byte aligned for Ethos-U NPU)
static uint8_t *g_model_data = nullptr;
static size_t g_model_data_size = 0;
static tflm_mem_src_t g_tensor_arena_src = TFLM_MEM_SRC_NONE;

/**
 * @brief Initialize the interpreter (called once)
 * @return bk_err_t BK_OK on success
 */
static bk_err_t tflm_init_interpreter(void)
{
    if (g_interpreter_initialized) {
        return BK_OK;  // Already initialized
    }

    //uint32_t ethosu0_scratch_size = 0;
    if (g_ethosu0_scratch == nullptr) {
        g_ethosu0_scratch = tflm_scratch_alloc(ETHOSU0_SCRATCH_SIZE + 16, &g_ethosu0_scratch_src);
        if (g_ethosu0_scratch == nullptr) {
            LOGE("runtime alloc ethosu0_scratch failed, size=%d\r\n", ETHOSU0_SCRATCH_SIZE);
            return BK_FAIL;
        }
        LOGI("EthosU0 scratch allocated from %s\r\n", tflm_mem_src_name(g_ethosu0_scratch_src));
    }
    void* ethosu0_scratch_aligned = g_ethosu0_scratch ? (void*)(((uint32_t)g_ethosu0_scratch + 15) & ~15) : nullptr;

    // Initialize Ethos-U driver
    int init_result = bk_ethosu_init(ethosu0_scratch_aligned, ETHOSU0_SCRATCH_SIZE);
    if(init_result != BK_OK)
    {
        LOGE("Failed to initialize Ethos-U driver, ret=%d\r\n", init_result);
        if (g_ethosu0_scratch) {
            tflm_scratch_free(g_ethosu0_scratch, g_ethosu0_scratch_src);
            g_ethosu0_scratch = nullptr;
            g_ethosu0_scratch_src = TFLM_MEM_SRC_NONE;
        }
        return BK_FAIL;
    }

    LOGI("Ethos-U driver initialized successfully\r\n");

    // Load model data into PSRAM memory.
    // Note: Model data must be 16-byte aligned for Ethos-U NPU command stream requirements
    if (g_model_data == nullptr) {
        g_model_data_size = g_person_detection_model_vela_data_len;
        // Allocate extra 16 bytes to ensure we can align to 16-byte boundary
        // This is critical for Ethos-U NPU which requires command stream addresses to be 16-byte aligned
        g_model_data_raw = (uint8_t *)tflm_runtime_alloc(g_model_data_size + 16, &g_model_data_src);
        if (g_model_data_raw == nullptr) {
            LOGE("runtime alloc model data failed, size=%d\r\n", (int)(g_model_data_size + 16));
            return BK_FAIL;
        }

        // Align to 16-byte boundary for Ethos-U NPU requirements
        uintptr_t aligned_addr = ((uintptr_t)g_model_data_raw + 15U) & ~((uintptr_t)15U);
        g_model_data = (uint8_t *)aligned_addr;

        // Copy model data from Flash to aligned buffer
        os_memcpy(g_model_data, g_person_detection_model_vela_data, g_model_data_size);
        LOGI("Model data loaded to %s: %p, size=%d bytes\r\n", tflm_mem_src_name(g_model_data_src), g_model_data, (int)g_model_data_size);
    }

    // Allocate tensor arena from runtime-selected memory.
    if (g_tensor_arena == nullptr) {
        g_tensor_arena = (uint8_t *)tflm_runtime_alloc(TFLM_ARENA_SIZE, &g_tensor_arena_src);
        if (g_tensor_arena == nullptr) {
            LOGE("runtime alloc tensor arena failed, size=%d\r\n", TFLM_ARENA_SIZE);
            // Clean up model data if allocation failed
            if (g_model_data_raw != nullptr) {
                tflm_runtime_free(g_model_data_raw, g_model_data_src);
                g_model_data_raw = nullptr;
                g_model_data = nullptr;
                g_model_data_size = 0;
                g_model_data_src = TFLM_MEM_SRC_NONE;
            }
            return BK_FAIL;
        }
        LOGI("Tensor arena allocated from %s: %p, size=%d bytes\r\n", tflm_mem_src_name(g_tensor_arena_src), g_tensor_arena, TFLM_ARENA_SIZE);
    }

    // Map the model into a usable data structure
    // Use model data from dynamically allocated PSRAM memory.
    const tflite::Model *model = tflite::GetModel(g_model_data);
    if (!model) {
        LOGE("get model failed\r\n");
        // Clean up model data if model mapping failed
        if (g_model_data_raw != nullptr) {
            tflm_runtime_free(g_model_data_raw, g_model_data_src);
            g_model_data_raw = nullptr;
            g_model_data = nullptr;
            g_model_data_size = 0;
            g_model_data_src = TFLM_MEM_SRC_NONE;
        }
        return BK_FAIL;
    }
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        LOGE("model schema version mismatch: %d != %d\r\n", 
             (int)model->version(), TFLITE_SCHEMA_VERSION);
        // Clean up model data if version mismatch
        if (g_model_data_raw != nullptr) {
            tflm_runtime_free(g_model_data_raw, g_model_data_src);
            g_model_data_raw = nullptr;
            g_model_data = nullptr;
            g_model_data_size = 0;
            g_model_data_src = TFLM_MEM_SRC_NONE;
        }
        return BK_FAIL;
    }

    // Register required operations for person detection.
    static tflite::MicroMutableOpResolver<13> micro_op_resolver;
    micro_op_resolver.AddEthosU();

    // Build an interpreter to run the model.
    static tflite::MicroInterpreter static_interpreter(model,
                                                       micro_op_resolver,
                                                       g_tensor_arena,
                                                       TFLM_ARENA_SIZE);
    g_interpreter = &static_interpreter;

    if (g_interpreter->AllocateTensors() != kTfLiteOk) {
        LOGE("AllocateTensors failed\r\n");
        // Clean up model data if tensor allocation failed
        if (g_model_data_raw != nullptr) {
            tflm_runtime_free(g_model_data_raw, g_model_data_src);
            g_model_data_raw = nullptr;
            g_model_data = nullptr;
            g_model_data_size = 0;
            g_model_data_src = TFLM_MEM_SRC_NONE;
        }
        // Clean up tensor arena if allocation failed
        if (g_tensor_arena != nullptr) {
            tflm_runtime_free(g_tensor_arena, g_tensor_arena_src);
            g_tensor_arena = nullptr;
            g_tensor_arena_src = TFLM_MEM_SRC_NONE;
        }
        return BK_FAIL;
    }

    // Print memory usage information
    LOGI("=== TFLM Memory Usage Information ===\r\n");
    if (g_ethosu0_scratch != nullptr) {
        void* ethosu0_scratch_aligned = (void*)(((uint32_t)g_ethosu0_scratch + 15) & ~15);
        uint8_t* scratch_start = (uint8_t*)ethosu0_scratch_aligned;
        uint8_t* scratch_end = scratch_start + ETHOSU0_SCRATCH_SIZE;
        LOGI("EthosU0 Scratch: start=0x%p, end=0x%p, size=%d bytes\r\n", 
             scratch_start, scratch_end, ETHOSU0_SCRATCH_SIZE);
    } else {
        LOGI("EthosU0 Scratch: not allocated\r\n");
    }

    if (g_tensor_arena != nullptr) {
        uint8_t* arena_start = g_tensor_arena;
        uint8_t* arena_end = arena_start + TFLM_ARENA_SIZE;
        LOGI("Tensor Arena   : start=0x%p, end=0x%p, size=%d bytes\r\n", 
             arena_start, arena_end, TFLM_ARENA_SIZE);
    } else {
        LOGI("Tensor Arena   : not allocated\r\n");
    }

    if (g_model_data != nullptr) {
        uint8_t* model_start = g_model_data;
        uint8_t* model_end = model_start + g_model_data_size;
        LOGI("Model Data     : start=0x%p, end=0x%p, size=%d bytes (%s)\r\n", 
             model_start, model_end, (int)g_model_data_size, tflm_mem_src_name(g_model_data_src));
    } else {
        LOGI("Model Data     : not allocated\r\n");
    }
    LOGI("=====================================\r\n");

    g_interpreter_initialized = true;
    return BK_OK;
}

/**
 * @brief Deinitialize the interpreter and release all resources
 * @return bk_err_t BK_OK on success
 */
static bk_err_t tflm_deinit_interpreter(void)
{
    if (!g_interpreter_initialized) {
        return BK_OK;  // Already deinitialized
    }

    // Reset interpreter pointer
    g_interpreter = nullptr;

    // Free tensor arena memory
    if (g_tensor_arena != nullptr) {
        tflm_runtime_free(g_tensor_arena, g_tensor_arena_src);
        g_tensor_arena = nullptr;
        g_tensor_arena_src = TFLM_MEM_SRC_NONE;
    }

    // Free model data memory
    // Must free the raw pointer, not the aligned pointer
    if (g_model_data_raw != nullptr) {
        tflm_runtime_free(g_model_data_raw, g_model_data_src);
        LOGI("Model data freed from %s\r\n", tflm_mem_src_name(g_model_data_src));
        g_model_data_raw = nullptr;
        g_model_data = nullptr;
        g_model_data_size = 0;
        g_model_data_src = TFLM_MEM_SRC_NONE;
    }

    // Deinitialize Ethos-U driver
    ethosu_deinit(&ethosu0_driver);

    // Free scratch memory
    if (g_ethosu0_scratch != nullptr) {
        tflm_scratch_free(g_ethosu0_scratch, g_ethosu0_scratch_src);
        g_ethosu0_scratch = nullptr;
        g_ethosu0_scratch_src = TFLM_MEM_SRC_NONE;
    }

    // Reset initialization flag
    g_interpreter_initialized = false;

    LOGI("Interpreter deinitialized successfully\r\n");
    return BK_OK;
}

/**
 * @brief Run gesture detection on a test image
 * @param image_type Image type: 0=paper, 1=rock, 2=scissors
 * @return bk_err_t BK_OK on success
 */
static bk_err_t tflm_run_one_person(int image_type)
{
    // Initialize interpreter if not already done
    bk_err_t ret = tflm_init_interpreter();
    if (ret != BK_OK) {
        LOGE("tflm_init_interpreter failed, ret=%d\r\n", ret);
        return ret;
    }

    PERSON_DETECTION_START();

    // Get input tensor
    TfLiteTensor *input = g_interpreter->input(0);
    if (!input) {
        LOGE("input tensor is null\r\n");
        return BK_FAIL;
    }

    // Verify input properties
    if (input->dims->size != 4 ||
        input->dims->data[1] != kNumRows ||
        input->dims->data[2] != kNumCols ||
        input->dims->data[3] != kNumChannels ||
        input->type != kTfLiteInt8) {
        LOGE("unexpected input properties\r\n");
        return BK_FAIL;
    }

    // Get image data from provider
    const char *person_names[] = {"person", "no person"};
    LOGI("Start detecting %s\r\n", person_names[image_type]);

    PERSON_DETECTION_GET_IMAGE_START();
    if (kTfLiteOk != GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8, image_type)) {
        LOGE("Image capture failed\r\n");
        return BK_FAIL;
    }
    PERSON_DETECTION_GET_IMAGE_END();

    // Run inference
    PERSON_DETECTION_INFERENCE_START();
#ifdef CONFIG_PERSON_DETECTION_INFER_TIME_AUTO
    {
        dwt_init_cycle_counter();
        uint32_t start_cycles = dwt_get_cycle_counter_val();
        TfLiteStatus invoke_ret = g_interpreter->Invoke();
        uint32_t end_cycles = dwt_get_cycle_counter_val();

        uint32_t cycles = end_cycles - start_cycles;
        uint32_t us = cycles / 480U;
        if (kTfLiteOk != invoke_ret) {
            LOGE("Invoke failed\r\n");
            return BK_FAIL;
        }
        if (us < 1000U) {
            LOGI("Invoke time: %u us\r\n", us);
        } else {
            uint32_t ms = us / 1000U;
            LOGI("Invoke time: %u ms\r\n", ms);
        }
    }
#else
    if (kTfLiteOk != g_interpreter->Invoke()) {
        LOGE("Invoke failed\r\n");
        return BK_FAIL;
    }
#endif
    PERSON_DETECTION_INFERENCE_END();

    PERSON_DETECTION_RESULT_PROCESS_START();
    // Get output tensor
    TfLiteTensor *output = g_interpreter->output(0);
    if (!output) {
        LOGE("output tensor is null\r\n");
        return BK_FAIL;
    }
    // Process the inference results.
    int8_t person_score = output->data.uint8[kPersonIndex];
    //int8_t no_person_score = output->data.uint8[kNotAPersonIndex];
    float person_score_f = (person_score - output->params.zero_point) * output->params.scale;
    //float no_person_score_f = (no_person_score - output->params.zero_point) * output->params.scale;    
    int person_score_int = (person_score_f) * 100 + 0.5;
    PERSON_DETECTION_RESULT_PROCESS_END();
    LOGI("person score:%d%%, no person score %d%%\r\n", person_score_int, 100 - person_score_int);

    PERSON_DETECTION_END();

    return BK_OK;
}

bk_err_t tflm_person_detection_run_demo(void)
{
    bk_err_t ret;

    PERSON_DETECTION_DEBUG_INIT();

    // Test paper image
    ret = tflm_run_one_person(0);
    if (ret != BK_OK) {
        LOGE("person test failed, ret=%d\r\n", ret);
        return ret;
    }

    // Test rock image
    ret = tflm_run_one_person(1);
    if (ret != BK_OK) {
        LOGE("no person test failed, ret=%d\r\n", ret);
        return ret;
    }

    LOGI("person detection demo finished successfully\r\n");
    return BK_OK;
}

