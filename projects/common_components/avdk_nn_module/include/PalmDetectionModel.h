#pragma once

#include "AvdkDetectionModel.h"

class PalmDetectionModel : public AvdkDetectionModel
{
public:
    void resolverLoad(void);
    void resourceLoad(void);
    void resourceUnload(void);
    int run(uint8_t *data, uint32_t size, bk_pixel_format_t format);
};
