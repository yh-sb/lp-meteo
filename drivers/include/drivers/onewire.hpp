#pragma once

#include <stdint.h>
#include "periph/uart.hpp"
#include "FreeRTOS.h"
#include "semphr.h"

namespace drv
{
class onewire
{
public:
    enum res_t
    {
        RES_OK        =  0,
        RES_LINE_BUSY = -1,
        RES_NO_DEV    = -2,
        RES_TX_FAIL   = -4,
        RES_RX_FAIL   = -5
    };
    
    onewire(periph::uart &uart);
    ~onewire();
    
    int8_t tx(uint64_t rom, void *tx_buff, uint16_t tx_size);
    int8_t rx(uint64_t rom, void *rx_buff, uint16_t rx_size);
    int8_t exch(uint64_t rom, void *tx_buff, uint16_t tx_size,
        void *rx_buff, uint16_t rx_size);
    int8_t read_rom(uint64_t *rom);
    //int8_t search(uint64_t *rom_list, size_t *rom_list_size);

private:
    int8_t do_reset();
    int8_t send_buff(void *buff, uint8_t size);
    int8_t read_buff(void *buff, uint8_t size);
    int8_t send_byte(uint8_t byte);
    int8_t read_byte(uint8_t *byte);
    
    periph::uart &_uart;
    SemaphoreHandle_t api_lock;
};
}
