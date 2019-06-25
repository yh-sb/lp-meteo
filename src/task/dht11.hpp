#pragma once

#include "drv/dht11/dht11.hpp"

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
    drv::dht11 *_dht11;
};

void dht11(void *pvParameters);

#ifdef __cplusplus 
}
#endif 
}
