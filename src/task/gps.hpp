#pragma once

#include "drv/gps/nmea.hpp"
#include "drv/hd44780/hd44780.hpp"
#include "FreeRTOS.h"
#include "queue.h"

namespace task
{
#ifdef __cplusplus
extern "C" {
#endif

struct gps_ctx_t
{
    drv::nmea *nmea;
    drv::hd44780 *lcd;
};

void gps(void *pvParameters);

#ifdef __cplusplus
}
#endif
}
