#include <stddef.h>
#include <cstring>
#include <assert.h>
#include "drivers/ds18b20.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace drv;

/* DS18B20 commands */
enum
{
    CMD_CONVERT_T        = 0x44, /* Start temperature conversion */
    CMD_WRITE_SCRATCHPAD = 0x4E, /* Transmit 3 bytes data to DS18B20 */
    CMD_READ_SCRATCHPAD  = 0xBE, /* Receive 9 bytes from DS18B20 */
    CMD_COPY_SCRATCHPAD  = 0x48, /* Copy Th, Tl and conf reg. to EEPROM */
    CMD_RECALL_E2        = 0xB8, /* Restore Th, Tl and conf reg from EEPROM */
    CMD_READ_POWER       = 0xB4  /* Get power supply status from DS18B20 */
};

/* DS18B20 scratchpad map */
enum
{
    SCRATCHPAD_TEMP_LSB,
    SCRATCHPAD_TEMP_MSB,
    SCRATCHPAD_HIGH_ALARM_TEMP,
    SCRATCHPAD_LOW_ALARM_TEMP,
    SCRATCHPAD_CONFIG,
    SCRATCHPAD_INTERNAL_BYTE,
    SCRATCHPAD_COUNT_REMAIN,
    SCRATCHPAD_COUNT_PER_C,
    SCRATCHPAD_CRC,
    SCRATCHPAD_TOTAL_SIZE /* End enum value. Total size of scratchpad */
};

/* Maximum conversion time for each resolution in ms */
static uint16_t time_list[ds18b20::RESOL_12_BIT + 1] = {94, 188, 375, 750};
static uint8_t conf_list[ds18b20::RESOL_12_BIT + 1] = {0x1F, 0x3F, 0x5F, 0x7F};

static const uint8_t crc8_table[256] =
{
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20,
    0xA3, 0xFD, 0x1F, 0x41, 0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E,
    0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC, 0x23, 0x7D, 0x9F, 0xC1,
    0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
    0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E,
    0x1D, 0x43, 0xA1, 0xFF, 0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5,
    0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07, 0xDB, 0x85, 0x67, 0x39,
    0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
    0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45,
    0xC6, 0x98, 0x7A, 0x24, 0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B,
    0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9, 0x8C, 0xD2, 0x30, 0x6E,
    0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
    0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31,
    0xB2, 0xEC, 0x0E, 0x50, 0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C,
    0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE, 0x32, 0x6C, 0x8E, 0xD0,
    0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
    0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA,
    0x69, 0x37, 0xD5, 0x8B, 0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4,
    0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16, 0xE9, 0xB7, 0x55, 0x0B,
    0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
    0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54,
    0xD7, 0x89, 0x6B, 0x35
};

static uint8_t calc_crc(void *buff, uint8_t size);
static float calc_temp(uint8_t temp_lsb, uint8_t temp_msb, uint8_t conf);

ds18b20::ds18b20(onewire &onewire):
    _onewire(onewire),
    _resol(RESOL_12_BIT)
{
    assert(api_lock = xSemaphoreCreateMutex());
}

ds18b20::~ds18b20()
{
    vSemaphoreDelete(api_lock);
}

int8_t ds18b20::get_temp(uint64_t rom, float *temp)
{
    assert(temp);
    
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t tx_byte = CMD_CONVERT_T;
    switch(_onewire.tx(rom, &tx_byte, 1))
    {
        case onewire::RES_LINE_BUSY:
        case onewire::RES_TX_FAIL:
        case onewire::RES_RX_FAIL: res = RES_ONEWIRE_BUSY; goto Exit;
        case onewire::RES_NO_DEV: res = RES_NO_DEV; goto Exit;
    }
    vTaskDelay(time_list[_resol]);
    
    uint8_t rx_buff[SCRATCHPAD_TOTAL_SIZE];
    res = read_scratchpad(rom, rx_buff, sizeof(rx_buff));
    if(res)
        goto Exit;
    
    *temp = calc_temp(rx_buff[SCRATCHPAD_TEMP_LSB],
        rx_buff[SCRATCHPAD_TEMP_MSB], rx_buff[SCRATCHPAD_CONFIG]);
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::set_resol(uint64_t rom, resol_t resol)
{
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t rx_buff[SCRATCHPAD_TOTAL_SIZE];
    res = read_scratchpad(rom, rx_buff, sizeof(rx_buff));
    if(res)
        goto Exit;
    
    if(rx_buff[SCRATCHPAD_CONFIG] == conf_list[resol])
    {
        _resol = resol;
        goto Exit;
    }
    
    res = write_scratchpad(rom, rx_buff[SCRATCHPAD_HIGH_ALARM_TEMP],
        rx_buff[SCRATCHPAD_LOW_ALARM_TEMP], conf_list[resol]);
    if(res)
        goto Exit;
    
    _resol = resol;
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::get_resol(uint64_t rom, resol_t *resol)
{
    assert(resol);
    
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t rx_buff[SCRATCHPAD_TOTAL_SIZE];
    res = read_scratchpad(rom, rx_buff, sizeof(rx_buff));
    if(res)
        goto Exit;
    
    if(rx_buff[SCRATCHPAD_CONFIG] == conf_list[RESOL_9_BIT])
        *resol = RESOL_9_BIT;
    else if(rx_buff[SCRATCHPAD_CONFIG] == conf_list[RESOL_10_BIT])
        *resol = RESOL_10_BIT;
    else if(rx_buff[SCRATCHPAD_CONFIG] == conf_list[RESOL_11_BIT])
        *resol = RESOL_11_BIT;
    else if(rx_buff[SCRATCHPAD_CONFIG] == conf_list[RESOL_12_BIT])
        *resol = RESOL_12_BIT;
    else
        assert(0);
    
    _resol = *resol;
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::set_th(uint64_t rom, uint8_t th)
{
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t rx_buff[SCRATCHPAD_TOTAL_SIZE];
    res = read_scratchpad(rom, rx_buff, sizeof(rx_buff));
    if(res)
        goto Exit;
    
    if(rx_buff[SCRATCHPAD_HIGH_ALARM_TEMP] == th)
        goto Exit;
    
    res = write_scratchpad(rom, th, rx_buff[SCRATCHPAD_HIGH_ALARM_TEMP],
        rx_buff[SCRATCHPAD_CONFIG]);
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::get_th(uint64_t rom, uint8_t *th)
{
    assert(th);
    
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t rx_buff[SCRATCHPAD_TOTAL_SIZE];
    res = read_scratchpad(rom, rx_buff, sizeof(rx_buff));
    if(res)
        goto Exit;
    
    *th = rx_buff[SCRATCHPAD_HIGH_ALARM_TEMP];

Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::set_tl(uint64_t rom, uint8_t tl)
{
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t rx_buff[SCRATCHPAD_TOTAL_SIZE];
    res = read_scratchpad(rom, rx_buff, sizeof(rx_buff));
    if(res)
        goto Exit;
    
    if(rx_buff[SCRATCHPAD_LOW_ALARM_TEMP] == tl)
        goto Exit;
    
    res = write_scratchpad(rom, tl, rx_buff[SCRATCHPAD_LOW_ALARM_TEMP],
        rx_buff[SCRATCHPAD_CONFIG]);
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::get_tl(uint64_t rom, uint8_t *tl)
{
    assert(tl);
    
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t rx_buff[SCRATCHPAD_TOTAL_SIZE];
    res = read_scratchpad(rom, rx_buff, sizeof(rx_buff));
    if(res)
        goto Exit;
    
    *tl = rx_buff[SCRATCHPAD_LOW_ALARM_TEMP];
    
Exit:
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::write_eeprom(uint64_t rom)
{
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t tx_byte = CMD_COPY_SCRATCHPAD;
    switch(_onewire.tx(rom, &tx_byte, 1))
    {
        case onewire::RES_LINE_BUSY:
        case onewire::RES_TX_FAIL:
        case onewire::RES_RX_FAIL: res = RES_ONEWIRE_BUSY; break;
        case onewire::RES_NO_DEV: res = RES_NO_DEV; break;
    }
    
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::restore_eeprom(uint64_t rom)
{
    xSemaphoreTake(api_lock, portMAX_DELAY);
    
    int8_t res = RES_OK;
    uint8_t tx_byte = CMD_RECALL_E2;
    switch(_onewire.tx(rom, &tx_byte, 1))
    {
        case onewire::RES_LINE_BUSY:
        case onewire::RES_TX_FAIL:
        case onewire::RES_RX_FAIL: res = RES_ONEWIRE_BUSY; break;
        case onewire::RES_NO_DEV: res = RES_NO_DEV; break;
    }
    
    xSemaphoreGive(api_lock);
    return res;
}

int8_t ds18b20::write_scratchpad(uint64_t rom, uint8_t th, uint8_t tl, uint8_t conf)
{
    uint8_t tx_buff[] = {CMD_WRITE_SCRATCHPAD, th, tl, conf};
    switch(_onewire.tx(rom, tx_buff, sizeof(tx_buff)))
    {
        case onewire::RES_LINE_BUSY:
        case onewire::RES_TX_FAIL:
        case onewire::RES_RX_FAIL: return RES_ONEWIRE_BUSY;
        case onewire::RES_NO_DEV: return RES_NO_DEV;
    }
    return RES_OK;
}

int8_t ds18b20::read_scratchpad(uint64_t rom, void *rx_buff, uint8_t rx_size)
{
    assert(rx_size >= SCRATCHPAD_TOTAL_SIZE);
    
    uint8_t cmd = CMD_READ_SCRATCHPAD;
    switch(_onewire.exch(rom, &cmd, 1, rx_buff, SCRATCHPAD_TOTAL_SIZE))
    {
        case onewire::RES_LINE_BUSY:
        case onewire::RES_TX_FAIL:
        case onewire::RES_RX_FAIL: return RES_ONEWIRE_BUSY;
        case onewire::RES_NO_DEV: return RES_NO_DEV;
    }
    if(calc_crc(rx_buff, SCRATCHPAD_TOTAL_SIZE))
        return RES_CRC_ERR;
    
    return RES_OK;
}

static uint8_t calc_crc(void *buff, uint8_t size)
{
    uint8_t crc = 0;
    uint8_t *_buff = static_cast<uint8_t *>(buff);
    
    while(size--)
        crc = crc8_table[crc ^ *_buff++];
    
    /* Calc CRC without table
    uint8_t byte, mix;
    while(size--)
    {
        byte = *_buff++;
        for(uint8_t i = 8; i; i--)
        {
            mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if(mix)
                crc ^= 0x8C;
            byte >>= 1;
        }
    }
    */
    
    return crc;
}

static float calc_temp(uint8_t temp_lsb, uint8_t temp_msb, uint8_t conf)
{
    int16_t temp = (temp_msb << 8) | temp_lsb;
    
    if(conf == conf_list[ds18b20::RESOL_12_BIT])
        return (float)temp * 0.0625;
    else if(conf == conf_list[ds18b20::RESOL_11_BIT])
        return (float)(temp >> 1) * 0.125;
    else if(conf == conf_list[ds18b20::RESOL_10_BIT])
        return (float)(temp >> 2) * 0.25;
    else if(conf == conf_list[ds18b20::RESOL_9_BIT])
        return (float)(temp >> 3) * 0.5;
    else
        assert(0);

    return 0;
}
