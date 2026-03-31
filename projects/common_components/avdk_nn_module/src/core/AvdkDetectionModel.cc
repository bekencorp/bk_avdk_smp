#include "AvdkDetectionModel.h"
#include "os/mem.h"
#include "os/str.h"
#include "os/os.h"
#include "components/bk_frame_buffer.h"

#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"

#include "bk_ethosu.h"


static const char* TAG = "det-model";

#define LOGI(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE((char*)TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD((char*)TAG, ##__VA_ARGS__)

AvdkDetectionModel::AvdkDetectionModel()
{

}

void *AvdkDetectionModel::allocMemory(avdk_nn_mem_type_t type, uint32_t size)
{
    if (type == AVDK_NN_MEM_TYPE_HSRAM)
    {
        return hsram_malloc(size);
    }
    else if (type == AVDK_NN_MEM_TYPE_SRAM)
    {
        return os_sram_malloc(size);
    }
    else if (type == AVDK_NN_MEM_TYPE_PSRAM_HEAP)
    {
        return psram_malloc(size);
    }
    else if (type == AVDK_NN_MEM_TYPE_PSRAM_SLAB)
    {
        return bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size);
    }

    LOGE("Unsupported memory type alloc\n");
    return NULL;
}

void AvdkDetectionModel::freeMemory(avdk_nn_mem_type_t type, void *ptr)
{
    if (type == AVDK_NN_MEM_TYPE_HSRAM)
    {
        hsram_free(ptr);
    }

    else if (type == AVDK_NN_MEM_TYPE_SRAM)
    {
        os_sram_free(ptr);
    }
    else if (type == AVDK_NN_MEM_TYPE_PSRAM_HEAP)
    {
        psram_free(ptr);
    }
    else if (type == AVDK_NN_MEM_TYPE_PSRAM_SLAB)
    {
        bk_frame_buffer_free(ptr);
    }
    else
    {
        LOGE("Unsupported memory type free\n");
    }
}

int AvdkDetectionModel::LoadModel()
{
    LOGI("AvdkDetectionModel::LoadModel\n");

    resourceLoad();

    if (model_ram_type == AVDK_NN_MEM_TYPE_FALSH)
    {
        model_data = model_flash_data;
        model_data_size = model_flash_data_size;
    }
    else
    {
        model_data_size = model_flash_data_size;
        model_data = (uint8_t*)allocMemory(model_ram_type, model_data_size);

        if (model_data == NULL)
        {
            LOGE("Failed to allocate model data\n");
            return -1;
        }

        os_memcpy(model_data, model_flash_data, model_flash_data_size);
    }

    return 0;
}

int AvdkDetectionModel::UnloadModel()
{
    if (model_ram_type != AVDK_NN_MEM_TYPE_FALSH && model_data != NULL)
    {
        freeMemory(model_ram_type, model_data);
        model_data = NULL;
        model_data_size = 0;
    }

    resourceUnload();

    return 0;
}

int AvdkDetectionModel::npuStartup()
{
    fast_ram_data = (uint8_t*)allocMemory(fast_ram_type, fast_ram_data_size + 16);

    if (fast_ram_data == NULL)
    {
      LOGE("Failed to allocate fast RAM\n");
      return -1;
    }

    void* aligned_ptr = fast_ram_data ? (void*)(((uint32_t)fast_ram_data + 15) & ~15) : nullptr;

    int ret = bk_ethosu_init(aligned_ptr, fast_ram_data_size);

    if(ret != 0)
    {
        LOGE("Failed to initialize Ethos-U driver, ret=%d\n", ret);
        if (fast_ram_data)
        {
            freeMemory(fast_ram_type, fast_ram_data);
            fast_ram_data = NULL;
        }
        return -1;
    }

    return 0;
}

int AvdkDetectionModel::npuShutdown()
{
    bk_ethosu_deinit();

    if (fast_ram_data)
    {
        freeMemory(fast_ram_type, fast_ram_data);
        fast_ram_data = NULL;
    }

    return 0;
}


int AvdkDetectionModel::cpuStartup()
{
    return 0;
}

int AvdkDetectionModel::cpuShutdown()
{

    return 0;
}

int AvdkDetectionModel::init()
{
    int ret = -1;

    ret = LoadModel();

    if (ret != 0)
    {
        LOGE("Failed to load model\n");
        return -1;
    }

    if (model_type == AVDK_NN_MODEL_TYPE_NPU)
    {
      ret = npuStartup();
    }
    else if (model_type == AVDK_NN_MODEL_TYPE_CPU)
    {
      ret = cpuStartup();
    }
    else
    {
      LOGE("Unsupported model type\n");
      return -1;
    }

    LOGI("AvdkDetectionModel::init %s\n", name);


    if (model_data == NULL)
    {
        LOGE("model_data is NULL\n");
        return -1;
    }

    const tflite::Model* model = ::tflite::GetModel(model_data);

    if(TFLITE_SCHEMA_VERSION != model->version())
    {
        LOGE("Model schema version mismatch\n");
        return -1;
    }

    resolverLoad();

    arena_ram_data = (uint8_t*)allocMemory(arena_ram_type, arena_data_size + 16);

    if (arena_ram_data == NULL)
    {
      LOGE("Failed to allocate arena RAM\n");
      goto error;
    }

    if(!(pinterpreter = new tflite::MicroInterpreter(model, micro_op_resolver, arena_ram_data, arena_data_size)))
    {
        ret = -3;
        goto error;
    }

    if(kTfLiteOk != pinterpreter->AllocateTensors())
    {
        ret = -4;
        goto error;
    }

    return ret;

error:

    if (model_type == AVDK_NN_MODEL_TYPE_NPU)
    {
        npuShutdown();
    }
    else if (model_type == AVDK_NN_MODEL_TYPE_CPU)
    {
        cpuShutdown();
    }

    if (arena_ram_data)
    {
        freeMemory(arena_ram_type, arena_ram_data);
        arena_ram_data = NULL;
    }

    if (pinterpreter)
    {
        delete pinterpreter;
        pinterpreter = NULL;
    }

    UnloadModel();

  return -1;
}

int AvdkDetectionModel::deinit()
{
    int ret = -1;

    if (model_type == AVDK_NN_MODEL_TYPE_NPU)
    {
        npuShutdown();
    }
    else if (model_type == AVDK_NN_MODEL_TYPE_CPU)
    {
        cpuShutdown();
    }

    ret = UnloadModel();
    if (ret != 0)
    {
        LOGE("Failed to unload model\n");
        return -1;
    }

    return 0;
}


static void debugLogCallback(const char* s){
    BK_LOGI((char*)"TFLM", "%s", s);
}

void AvdkDetectionModel::LogEnable(bool enable)
{
    if (enable)
    {
        RegisterDebugLogCallback(debugLogCallback);
    }
    else
    {
        RegisterDebugLogCallback(NULL);
    }
}

uint16_t AvdkDetectionModel::getWidth()
{
    return width;
}

uint16_t AvdkDetectionModel::getHeight()
{
    return height;
}

bk_pixel_format_t AvdkDetectionModel::getFormat()
{
    return format;
}