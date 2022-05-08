#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "periph/gpio.hpp"

namespace drv
{
class enc
{
typedef void (*enc_cb_t)(enc *enc, int8_t diff, void *ctx);

public:
    enc(periph::gpio &a, periph::gpio &b);
    ~enc();
    
    void cb(enc_cb_t cb, void *ctx) { _cb = cb; _ctx = ctx; };
    void poll();
    
private:
    periph::gpio &_a;
    periph::gpio &_b;
    uint8_t prev_state;
    uint8_t prev_prev_state;
    void *_ctx;
    enc_cb_t _cb;
};
}
