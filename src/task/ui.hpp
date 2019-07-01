#pragma once

#include "drv/dht/dht.hpp"
#include "drv/ds18b20/ds18b20.hpp"
#include "drv/hd44780/hd44780.hpp"
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

struct dht_data_t
{
    int8_t res;
    drv::dht::val_t val;
};

struct ds18b20_data_t
{
    int8_t err;
    float t;
};

struct ui_queue_t
{
    ui_cmd_t cmd;
    union
    {
        dht_data_t dht11;
        ds18b20_data_t ds18b20;
    };
};

struct ui_ctx_t
{
    xQueueHandle to_ui;
    drv::hd44780 *lcd;
};

void ui(void *pvParameters);

#ifdef __cplusplus
}
#endif
}
