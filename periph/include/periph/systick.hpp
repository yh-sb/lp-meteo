#pragma once

#include <stdint.h>

namespace periph
{
class systick
{
public:
    static void init(void);
    static uint32_t get_ms(void);
    static uint32_t get_past(uint32_t start);
    
private:
    systick() {}
};
}
