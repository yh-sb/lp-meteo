#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "periph/gpio.hpp"

namespace drv
{
class dot_matrix
{
public:
    struct res_t
    {
        size_t x_size;
        size_t y_size;
    };
    
    dot_matrix(size_t rows, size_t columns, periph::gpio *bits[],
        periph::gpio &clk, bool is_inverted = false);
    ~dot_matrix();
    
#pragma pack(push, 1)
    struct symbol_t
    {
        uint8_t rows;
        uint8_t columns;
        uint16_t code;
        bool data[8][6];
    };
#pragma pack(pop)
    
    void load_symbols(symbol_t *symbols, size_t size);
    
    res_t get_size(const char *format, ...);
    res_t print(size_t time_per_dot, const char *format, ...);
    res_t print_centered(size_t time_per_dot, const char *format, ...);
    res_t print(size_t time_per_dot, uint16_t symbol_code);
    
    void shift_x(size_t time_per_dot, size_t x);
    void clear();
    
private:
    periph::gpio *_bits[8];
    periph::gpio &_clk;
    size_t _rows;
    size_t _columns;
    bool _is_inverted;
    symbol_t *_symbols;
    uint16_t _symbols_size;
    
    symbol_t *get_symbol(uint16_t symbol_code);
    void print(size_t time_per_dot, symbol_t &symbol);
};
}
