#pragma once

#include "drv/dht/dht.hpp"
#include "FreeRTOS.h"
#include "queue.h"

namespace task
{
#ifdef __cplusplus
extern "C" {
#endif

struct dht11_ctx_t
{
    xQueueHandle to_ui;
    drv::dht *dht11;
};

void dht11(void *pvParameters);

#ifdef __cplusplus
}
#endif
}
