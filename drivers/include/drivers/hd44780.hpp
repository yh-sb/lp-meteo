#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "periph/gpio.hpp"
#include "periph/tim.hpp"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

namespace drv
{
class hd44780
{
public:
    hd44780(periph::gpio &rs, periph::gpio &rw, periph::gpio &e, periph::gpio &db4,
        periph::gpio &db5, periph::gpio &db6, periph::gpio &db7, periph::tim &tim);
    
    ~hd44780();
    
    void init();
    
    /**
     * @brief Print formatted text
     * 
     * @param ddram_addr DDRAM address to which text will be writed.
     *                   Example for 4 line display: 0 - 1st line,
     *                   64 - 2nd line, 20 - 3rd line, 84 - 4th line
     * @param format A string that specifies the format of the output
     * @param ... Arguments used by format string
     * @return uint8_t New DDRAM address after writing
     */
    uint8_t print(uint8_t ddram_addr, const char *format, ...);
    
    /**
     * @brief Print one byte
     * 
     * @param ddram_addr DDRAM address to which text will be writed
     * @param byte ASCII symbol
     * @return uint8_t New DDRAM address after writing
     */
    uint8_t print(uint8_t ddram_addr, char byte);
    
    uint8_t ddram_addr();
    
    void write_cgram(uint8_t buff[8][8]);
    void read_cgram(uint8_t buff[8][8]);
    
    void clear();
    
private:
    periph::gpio &_rs;
    periph::gpio &_rw;
    periph::gpio &_e;
    periph::gpio *_db[4];
    periph::tim &_tim;
    TaskHandle_t task;
    SemaphoreHandle_t api_lock;
    
    enum write_t
    {
        CMD,
        DATA
    };
    
    void write_4bit(uint8_t half_byte);
    void write(write_t type, uint8_t byte);
    
    uint8_t read_4bit();
    uint8_t read_bf_and_ddram_addr();
    
    static void tim_cb(periph::tim *tim, void *ctx);
    void delay(uint32_t us);
};
}
