#pragma once

#include "drivers/ds18b20.hpp"
#include "FreeRTOS.h"
#include "queue.h"

namespace tasks
{
#ifdef __cplusplus
extern "C" {
#endif

struct ds18b20_ctx_t
{
    QueueHandle_t to_ui;
    drv::ds18b20 *_ds18b20;
};

void ds18b20(void *pvParameters);

#ifdef __cplusplus
}
#endif
}
