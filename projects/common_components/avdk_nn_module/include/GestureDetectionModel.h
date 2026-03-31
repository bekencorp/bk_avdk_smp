#pragma once

#include "AvdkDetectionModel.h"

typedef enum {
    GESTURE_ROCK = 0,
    GESTURE_PAPER,
    GESTURE_SCISSORS,
    GESTURE_NONE,   /**< No valid gesture detected */
    GESTURE_MAX,    /**< Sentinel value, must be the last */
} gesture_result_t;

/**
 * @brief Callback type: (gesture) detected gesture result.
 * Each GestureDetectionModel instance holds its own callback via setResultCallback().
 */
typedef void (*gesture_result_callback_t)(gesture_result_t gesture);

/**
 * @brief Callback type for detected image data.
 *
 * This callback is invoked after gesture detection completes, providing the input image
 * that was fed to the model. The image data pointer is valid only during the callback
 * execution. If you need to keep the data, make a copy inside the callback.
 *
 * @param image_data Pointer to image pixel data (BGRA8888 format).
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param format Pixel format (bk_pixel_format_t, typically BK_PIXEL_FORMAT_BGRA8888).
 * @param data_size Total size of image data in bytes.
 */
typedef void (*gesture_image_callback_t)(const uint8_t *image_data, uint32_t width, uint32_t height, bk_pixel_format_t format, uint32_t data_size);

class GestureDetectionModel : public AvdkDetectionModel
{
public:
    GestureDetectionModel() : result_callback_(nullptr), image_callback_(nullptr) {}

    void resolverLoad(void);
    void resourceLoad(void);
    void resourceUnload(void);
    int run(uint8_t *data, uint32_t size);

    /**
     * @brief Set per-instance result callback. Enables multiple model instances with separate callbacks.
     * @param cb Callback (gesture); NULL to clear.
     */
    void setResultCallback(gesture_result_callback_t cb) { result_callback_ = cb; }

    /**
     * @brief Set per-instance image callback. The callback will be invoked after each detection
     *        with the input image data that was fed to the model.
     * @param cb Callback (image_data, width, height, format, data_size); NULL to clear.
     */
    void setImageCallback(gesture_image_callback_t cb) { image_callback_ = cb; }

protected:
    uint8_t post_process(int8_t *out_data, uint8_t *result);
    float g_scale;
    int32_t g_zero_point;

private:
    gesture_result_callback_t result_callback_;
    gesture_image_callback_t image_callback_;
};