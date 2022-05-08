#pragma once

#include <stdint.h>
#include "periph/gpio.hpp"
#include "periph/tim.hpp"
#include "periph/exti.hpp"
#include "FreeRTOS.h"
#include "task.h"

namespace drv
{
class singlewire
{
public:
    enum res_t
    {
        RES_OK      =  0,
        RES_NODEV   = -1,
        RES_DEVERR  = -2,
        RES_READERR = -3,
        RES_BUSY    = -4
    };
    
    singlewire(periph::gpio &gpio, periph::tim &tim, periph::exti &exti);
    ~singlewire();
    
    int8_t read(uint8_t *buff, uint16_t size);
    
private:
    periph::gpio &_gpio;
    periph::tim &_tim;
    periph::exti &_exti;
    TaskHandle_t task;
    int8_t res;
    
    struct
    {
        uint8_t state;
        uint8_t *buff;
        uint16_t size;
        uint16_t byte;
        uint8_t bit;
    } fsm;
    void fsm_start(uint8_t *buff, uint16_t size);
    void fsm_run(bool is_tim_expired);
    
    static void tim_cb(periph::tim *tim, void *ctx);
    static void exti_cb(periph::exti *exti, void *ctx);
};
}
