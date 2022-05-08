#include <string.h>
#include <assert.h>
#include "drivers/dot_matrix.hpp"
#include "printf.h"
#include "FreeRTOS.h"
#include "task.h"

using namespace drv;

dot_matrix::dot_matrix(size_t rows, size_t columns, periph::gpio *bits[],
    periph::gpio &clk, bool is_inverted):
    _clk(clk),
    _rows(rows),
    _columns(columns),
    _is_inverted(is_inverted),
    _symbols(nullptr),
    _symbols_size(0)
{
    memcpy(_bits, bits, _rows * sizeof(_bits[0]));
}

dot_matrix::~dot_matrix()
{
    
}

void dot_matrix::load_symbols(symbol_t *symbols, size_t size)
{
    _symbols = symbols;
    _symbols_size = size;
}

dot_matrix::res_t dot_matrix::get_size(const char *format, ...)
{
    res_t res = {};
    va_list args;
    va_start(args, format);
    
    char msg[200] = {};
    vsnprintf_(msg, sizeof(msg) - 1, format, args);
    for(uint8_t i = 0; msg[i] != '\0'; i++)
    {
        symbol_t *symbol = get_symbol(msg[i]);
        if(symbol)
        {
            res.x_size += symbol->columns;
            res.y_size += symbol->rows;
        }
    }
    
    va_end(args);
    return res;
}

dot_matrix::res_t dot_matrix::print(size_t time_per_dot, const char *format, ...)
{
    res_t res = {};
    va_list args;
    va_start(args, format);
    
    char msg[200] = {};
    vsnprintf_(msg, sizeof(msg) - 1, format, args);
    for(uint8_t i = 0; msg[i] != '\0'; i++)
    {
        res_t symbol_res = print(time_per_dot, msg[i]);
        res.x_size += symbol_res.x_size;
        res.y_size += symbol_res.y_size;
    }
    
    va_end(args);
    return res;
}

dot_matrix::res_t dot_matrix::print(size_t time_per_dot, uint16_t symbol_code)
{
    for(uint16_t i = 0; i < _symbols_size; i++)
    {
        if(_symbols[i].code == symbol_code)
        {
            print(time_per_dot, _symbols[i]);
            return { _symbols[i].columns, _symbols[i].rows };
        }
    }
    
    return {};
}

void dot_matrix::shift_x(size_t time_per_dot, size_t x)
{
    assert(x <= _columns);
    
    for(uint8_t column = 0; column < x; column++)
    {
        for(uint8_t row = 0; row < _rows; row++)
        {
            if(_is_inverted)
                _bits[row]->set(!0);
            else
                _bits[row]->set(0);
        }
        
        _clk.set(0);
        _clk.set(1);
        vTaskDelay(time_per_dot);
    }
}

void dot_matrix::clear()
{
    for(uint8_t column = 0; column < _columns; column++)
    {
        for(uint8_t row = 0; row < _rows; row++)
        {
            if(_is_inverted)
                _bits[row]->set(!0);
            else
                _bits[row]->set(0);
        }
        
        _clk.set(0);
        _clk.set(1);
    }
}

dot_matrix::symbol_t *dot_matrix::get_symbol(uint16_t symbol_code)
{
    for(uint16_t i = 0; i < _symbols_size; i++)
    {
        if(_symbols[i].code == symbol_code)
            return &_symbols[i];
    }
    
    return nullptr;
}

void dot_matrix::print(size_t time_per_dot, symbol_t &symbol)
{
    for(uint8_t column = 0; column < symbol.columns; column++)
    {
        for(uint8_t row = 0; row < symbol.rows; row++)
        {
            if(_is_inverted)
                _bits[row]->set(!symbol.data[row][column]);
            else
                _bits[row]->set(symbol.data[row][column]);
        }
        
        _clk.set(0);
        _clk.set(1);
        vTaskDelay(time_per_dot);
    }
}
