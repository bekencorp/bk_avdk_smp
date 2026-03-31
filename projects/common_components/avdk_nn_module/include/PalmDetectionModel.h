#pragma once

#include "AvdkDetectionModel.h"

/**
 * @brief Callback type: (has_palm, cx, cy, w, h) in model coordinates (e.g. 256x256).
 * Each PalmDetectionModel instance holds its own callback via setResultCallback().
 */
typedef void (*palm_result_callback_t)(int has_palm, float cx, float cy, float w, float h, uint64_t timestamp);

class PalmDetectionModel : public AvdkDetectionModel
{
public:
    PalmDetectionModel() : result_callback_(nullptr) {}

    void resolverLoad(void);
    void resourceLoad(void);
    void resourceUnload(void);
    int run(uint8_t *data, uint32_t size);

    /**
     * @brief Set per-instance result callback. Enables multiple model instances with separate callbacks.
     * @param cb Callback (has_palm, cx, cy, w, h); NULL to clear.
     */
    void setResultCallback(palm_result_callback_t cb) { result_callback_ = cb; }

private:
    palm_result_callback_t result_callback_;
};
