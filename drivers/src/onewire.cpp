#include <stddef.h>
#include <cstring>
#include <assert.h>
#include "drivers/onewire.hpp"

using namespace drv;
using namespace periph;

#define RX_WAIT_TIMEOUT 5 /* ms */

enum cmd_t
{
    CMD_SEARCH_ROM   = 0xF0, /* Search ROM */
    CMD_ALARM_SEARCH = 0xEC, /* Search ROM with alarm */
    CMD_READ_ROM     = 0x33, /* Read ROM of 1wire device. Only if single device
                                is present on 1 wire bus */
    CMD_MATCH_ROM    = 0x55, /* Select specific 1 wire device by it's ROM */
    CMD_SKIP_ROM     = 0xCC  /* Send/receive data to all devices on the bus */
};

onewire::onewire(uart &uart):
    _uart(uart)
{
    assert(api_lock = xSemaphoreCreateMutex());
}

onewire::~onewire()
{
    
}

int8_t onewire::tx(uint64_t rom, void *tx_buff, uint16_t tx_size)
{
    assert(tx_buff);
    assert(tx_size > 0);
    
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = do_reset();
    if(res)
        goto Exit;
    
    if(!rom)
        res = send_byte(CMD_SKIP_ROM);
    else
    {
        uint8_t rom_buff[9];
        rom_buff[0] = CMD_MATCH_ROM;
        for(uint8_t i = 1; i < sizeof(rom_buff); i++, rom = rom >> 8)
            rom_buff[i] = rom;
        res = send_buff(rom_buff, sizeof(rom_buff));
    }
    if(res)
        goto Exit;
    
    res = send_buff(tx_buff, tx_size);
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t onewire::rx(uint64_t rom, void *rx_buff, uint16_t rx_size)
{
    assert(rx_buff);
    assert(rx_size > 0);
    
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = do_reset();
    if(res)
        goto Exit;
    
    if(!rom)
        res = send_byte(CMD_SKIP_ROM);
    else
    {
        uint8_t rom_buff[9];
        rom_buff[0] = CMD_MATCH_ROM;
        for(uint8_t i = 1; i < sizeof(rom_buff); i++, rom = rom >> 8)
            rom_buff[i] = rom;
        res = send_buff(rom_buff, sizeof(rom_buff));
    }
    if(res)
        goto Exit;
    
    res = read_buff(rx_buff, rx_size);
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t onewire::exch(uint64_t rom, void *tx_buff, uint16_t tx_size, void *rx_buff,
    uint16_t rx_size)
{
    assert(tx_buff);
    assert(tx_size > 0);
    assert(rx_buff);
    assert(rx_size > 0);
    
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = do_reset();
    if(res)
        goto Exit;
    
    if(!rom)
        res = send_byte(CMD_SKIP_ROM);
    else
    {
        uint8_t rom_buff[9];
        rom_buff[0] = CMD_MATCH_ROM;
        for(uint8_t i = 1; i < sizeof(rom_buff); i++, rom = rom >> 8)
            rom_buff[i] = rom;
        res = send_buff(rom_buff, sizeof(rom_buff));
    }
    if(res)
        goto Exit;
    
    if(tx_buff)
    {
        res = send_buff(tx_buff, tx_size);
        if(res)
            goto Exit;
    }
    
    if(rx_buff)
        res = read_buff(rx_buff, rx_size);
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t onewire::read_rom(uint64_t *rom)
{
    assert(rom);
    
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = do_reset();
    if(res)
        goto Exit;
    
    res = send_byte(CMD_READ_ROM);
    if(res)
        goto Exit;
    
    uint8_t rx_buff[8];
    res = read_buff(rx_buff, 8);
    if(res)
        goto Exit;
    
    *rom = 0;
    for(uint8_t i = 0; i < sizeof(*rom); i++)
        *rom = (*rom << 8) | rx_buff[7 - i];
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

/*int8_t onewire::search(uint64_t *rom_list, size_t *rom_list_size)
{
    
}*/

int8_t onewire::do_reset()
{
    int8_t res = RES_OK;//, uart_res;
    uint8_t tx_buff = 0xF0, rx_buff = 0x00;
    uint16_t size = sizeof(rx_buff);
    
    if(_uart.baud() != 9600)
        _uart.baud(9600);
    
    if(_uart.write(&tx_buff, 1) != uart::RES_OK)
    {
        res = RES_TX_FAIL;
        goto Exit;
    }
    
    switch(_uart.read(&rx_buff, &size, RX_WAIT_TIMEOUT))
    {
        case uart::RES_RX_TIMEOUT: res = RES_NO_DEV; break;
        case uart::RES_RX_FAIL: res = RES_RX_FAIL; break;
        case uart::RES_TX_FAIL: res = RES_TX_FAIL; break;
        default:
            if(size != sizeof(rx_buff))
                res = RES_RX_FAIL;
            else if(rx_buff == 0x00)
                res = RES_LINE_BUSY;
    }
    
    /*uart_res = _uart.read(&rx_buff, &size, RX_WAIT_TIMEOUT);
    if(uart_res == uart::RES_RX_TIMEOUT)
        res = RES_NO_DEV;
    else if(uart_res == uart::RES_RX_FAIL || size != sizeof(rx_buff))
        res = RES_RX_FAIL;
    else if(uart_res == uart::RES_TX_FAIL)
        res = RES_TX_FAIL;
    else if(rx_buff == 0x00)
        res = RES_LINE_BUSY;*/
    
Exit:
    _uart.baud(115200);
    return res;
}

int8_t onewire::send_buff(void *buff, uint8_t size)
{
    int8_t res = RES_OK;
    
    for(uint8_t i = 0; (i < size) && (res == RES_OK); i++)
        res = send_byte(static_cast<uint8_t *>(buff)[i]);
    
    return res;
}

int8_t onewire::read_buff(void *buff, uint8_t size)
{
    int8_t res = RES_OK;
    
    for(uint8_t i = 0; (i < size) && (res == RES_OK); i++)
        res = read_byte(&static_cast<uint8_t *>(buff)[i]);
    
    return res;
}

int8_t onewire::send_byte(uint8_t byte)
{
    int8_t res = RES_OK;
    uint8_t tx_buff[8], rx_buff[8];
    uint16_t size = sizeof(rx_buff);
    
    for(uint8_t i = 0; i < size; i++)
        tx_buff[i] = ((byte >> i) & 1) ? 0xFF : 0x00;
    
    switch(_uart.exch(tx_buff, size, rx_buff, &size, RX_WAIT_TIMEOUT))
    {
        case uart::RES_RX_TIMEOUT: res = RES_NO_DEV; break;
        case uart::RES_RX_FAIL: res = RES_RX_FAIL; break;
        case uart::RES_TX_FAIL: res = RES_TX_FAIL; break;
        default:
            if(size != sizeof(rx_buff))
                res = RES_RX_FAIL;
            else if(rx_buff == 0x00)
                res = RES_LINE_BUSY;
    }
    
    if(res == RES_OK)
    {
        if(memcmp(tx_buff, rx_buff, sizeof(tx_buff)))
            res = RES_LINE_BUSY;
    }
    
    return res;
}

int8_t onewire::read_byte(uint8_t *byte)
{
    int8_t res = RES_OK;
    uint8_t tx_buff[8], rx_buff[8];
    uint16_t size = sizeof(rx_buff);
    
    memset(tx_buff, 0xFF, sizeof(tx_buff));
    
    switch(_uart.exch(tx_buff, size, rx_buff, &size, RX_WAIT_TIMEOUT))
    {
        case uart::RES_RX_TIMEOUT: res = RES_NO_DEV; break;
        case uart::RES_RX_FAIL: res = RES_RX_FAIL; break;
        case uart::RES_TX_FAIL: res = RES_TX_FAIL; break;
        default:
            if(size != sizeof(rx_buff))
                res = RES_RX_FAIL;
            else if(rx_buff == 0x00)
                res = RES_LINE_BUSY;
    }
    
    if(res == RES_OK)
    {
        *byte = 0;
        for(uint8_t i = 0; i < size; i++)
            *byte |= (rx_buff[i] == 0xFF) << i;
    }
    
    return res;
}
