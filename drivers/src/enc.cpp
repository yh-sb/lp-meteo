#include <stddef.h>
#include <assert.h>
#include "drivers/enc.hpp"

using namespace drv;
using namespace periph;

enum
{
    STATE_1_1 = 0b11,
    STATE_0_1 = 0b01,
    STATE_0_0 = 0b00,
    STATE_1_0 = 0b10
};

enc::enc(gpio &a, gpio &b):
    _a(a),
    _b(b),
    prev_state(STATE_0_0),
    prev_prev_state(STATE_0_0),
    _ctx(NULL),
    _cb(NULL)
{
    assert(_a.mode() == gpio::mode::DI);
    assert(_b.mode() == gpio::mode::DI);
}

enc::~enc()
{
    
}

void enc::poll()
{
    uint8_t state = (_a.get() << 1) | _b.get();
    if(state == prev_state)
        return;
    
    if(prev_state == STATE_1_1 && state != prev_prev_state && _cb)
        _cb(this, (state == STATE_1_0) ? +1 : -1, _ctx);
    
    prev_prev_state = prev_state;
    prev_state = state;
}
