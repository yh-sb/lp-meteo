#pragma once

#include "drivers/nmea_reader.hpp"
#include "drivers/hd44780.hpp"
#include "FreeRTOS.h"
#include "queue.h"

namespace tasks
{
#ifdef __cplusplus
extern "C" {
#endif

struct gps_ctx_t
{
    drv::nmea_reader *nmea;
};

void gps(void *pvParameters);

#ifdef __cplusplus
}
#endif
}
