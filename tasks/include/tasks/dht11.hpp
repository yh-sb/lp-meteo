#pragma once

#include "drivers/dht.hpp"
#include "FreeRTOS.h"
#include "queue.h"

namespace tasks
{
#ifdef __cplusplus
extern "C" {
#endif

struct dht11_ctx_t
{
    QueueHandle_t to_ui;
    drv::dht *dht11;
};

void dht11(void *pvParameters);

#ifdef __cplusplus
}
#endif
}
