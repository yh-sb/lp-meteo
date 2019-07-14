#pragma once

#include "drv/ds18b20/ds18b20.hpp"
#include "FreeRTOS.h"
#include "queue.h"

namespace task
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
