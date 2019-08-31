#pragma once

#include "drv/dht/dht.hpp"
#include "drv/ds18b20/ds18b20.hpp"
#include "drv/dot_matrix/dot_matrix.hpp"
#include "FreeRTOS.h"
#include "queue.h"

namespace task
{
#ifdef __cplusplus
extern "C" {
#endif

enum ui_cmd_t
{
    CMD_DHT11,
    CMD_DS18B20
};

struct ui_queue_t
{
    ui_cmd_t cmd;
    union
    {
        drv::dht::val_t dht;
        float t;
    };
};

struct ui_ctx_t
{
    QueueHandle_t to_ui;
    drv::dot_matrix *matrix;
};

void ui(void *pvParameters);

#ifdef __cplusplus
}
#endif
}
