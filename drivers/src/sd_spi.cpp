#include <stddef.h>
#include <cstring>
#include <assert.h>
#include "drivers/sd_spi.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace drv;
using namespace periph;

#define WAIT_RESPONSE_CNT        10

#define TRANSMITTER_BIT 6
#define END_BIT 0

#define DATA_TOKEN_CMD17_18_24 0xFE
#define DATA_TOKEN_CMD25       0xFC
#define STOP_TOKEN_CMD25       0xFD

#define ERR_TOKEN_MASK             0x1F     /* 7, 6, 5 bits are not used */
#define ERR_TOKEN_ERR              (1 << 0)
#define ERR_TOKEN_CC_ERR           (1 << 1)
#define ERR_TOKEN_ECC_FAIL         (1 << 2)
#define ERR_TOKEN_OUT_OF_RANGE_ERR (1 << 3)
#define ERR_TOKEN_CARD_LOCKED      (1 << 4)

#define DATA_RESP_MASK      0x1F     /* 7, 6, 5 bits are not used */
#define DATA_RESP_START_BIT (1 << 0) /* Should be 1 */
#define DATA_RESP_END_BIT   (1 << 4) /* Should be 0 */
#define DATA_RESP_ACCEPTED  0x05     /* Data accepted */
#define DATA_RESP_CRC_ERR   0x0B     /* Data rejected due to a CRC erro */
#define DATA_RESP_WRITE_ERR 0x0D     /* Data rejected due to a write error */

#define R1_START_BIT 0x80

#define CRC7_CID_CSD_MASK 0xFE

/* crc7 table (crc8 table shifted left by 1 bit ) */
static const uint8_t crc7_table[256] =
{
    0x00, 0x12, 0x24, 0x36, 0x48, 0x5A, 0x6C, 0x7E,
    0x90, 0x82, 0xB4, 0xA6, 0xD8, 0xCA, 0xFC, 0xEE,
    0x32, 0x20, 0x16, 0x04, 0x7A, 0x68, 0x5E, 0x4C,
    0xA2, 0xB0, 0x86, 0x94, 0xEA, 0xF8, 0xCE, 0xDC,
    0x64, 0x76, 0x40, 0x52, 0x2C, 0x3E, 0x08, 0x1A,
    0xF4, 0xE6, 0xD0, 0xC2, 0xBC, 0xAE, 0x98, 0x8A,
    0x56, 0x44, 0x72, 0x60, 0x1E, 0x0C, 0x3A, 0x28,
    0xC6, 0xD4, 0xE2, 0xF0, 0x8E, 0x9C, 0xAA, 0xB8,
    0xC8, 0xDA, 0xEC, 0xFE, 0x80, 0x92, 0xA4, 0xB6,
    0x58, 0x4A, 0x7C, 0x6E, 0x10, 0x02, 0x34, 0x26,
    0xFA, 0xE8, 0xDE, 0xCC, 0xB2, 0xA0, 0x96, 0x84,
    0x6A, 0x78, 0x4E, 0x58, 0x22, 0x30, 0x06, 0x14,
    0xAC, 0xBE, 0x88, 0x9A, 0xE4, 0xF6, 0xC0, 0xD2,
    0x3C, 0x2E, 0x18, 0x0A, 0x74, 0x66, 0x50, 0x42,
    0x9E, 0x8C, 0xBA, 0xA8, 0xD6, 0xC4, 0xF2, 0xE0,
    0x0E, 0x1C, 0x2A, 0x38, 0x46, 0x54, 0x62, 0x70,
    0x82, 0x90, 0xA6, 0xB4, 0xCA, 0xD8, 0xEE, 0xFC,
    0x12, 0x00, 0x36, 0x24, 0x5A, 0x48, 0x7E, 0x6C,
    0xB0, 0xA2, 0x94, 0x86, 0xF8, 0xEA, 0xDC, 0xCE,
    0x20, 0x32, 0x04, 0x16, 0x68, 0x7A, 0x4C, 0x5E,
    0xE6, 0xF4, 0xC2, 0xD0, 0xAE, 0xBC, 0x8A, 0x98,
    0x76, 0x64, 0x52, 0x40, 0x3E, 0x2C, 0x1A, 0x08,
    0xD4, 0xC6, 0xF0, 0xE2, 0x9C, 0x8E, 0xB8, 0xAA,
    0x44, 0x56, 0x60, 0x72, 0x0C, 0x1E, 0x28, 0x3A,
    0x4A, 0x58, 0x6E, 0x7C, 0x02, 0x10, 0x26, 0x34,
    0xDA, 0xC8, 0xFE, 0xEC, 0x92, 0x80, 0xB6, 0xA4,
    0x78, 0x6A, 0x5C, 0x4E, 0x30, 0x22, 0x14, 0x06,
    0xE8, 0xFA, 0xCC, 0xDE, 0xA0, 0xB2, 0x84, 0x96,
    0x2E, 0x3C, 0x0A, 0x18, 0x66, 0x74, 0x42, 0x50,
    0xBE, 0xAC, 0x9A, 0x88, 0xF6, 0xE4, 0xD2, 0xC0,
    0x1C, 0x0E, 0x38, 0x2A, 0x54, 0x46, 0x70, 0x62,
    0x8C, 0x9C, 0xA8, 0xBA, 0xC4, 0xD6, 0xE0, 0xF2
};

static const uint16_t crc16_ccitt_table[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static uint8_t calc_crc7(uint8_t *buff, uint8_t size);
static uint16_t calc_crc16(uint8_t *buff, uint16_t size);

sd_spi::sd_spi(periph::spi &spi, periph::gpio &cs, periph::gpio *cd):
    sd(cd),
    _spi(spi),
    _cs(cs)
{
    assert(_spi.cpol() == spi::CPOL_0);
    assert(_spi.cpha() == spi::CPHA_0);
    assert(_spi.bit_order() == spi::BIT_ORDER_MSB);
    assert(_cs.mode() == gpio::mode::DO && cs.get());
}

sd_spi::~sd_spi()
{
    
}

void sd_spi::select(bool is_selected)
{
    _cs.set(is_selected ? 0 : 1);
}

int8_t sd_spi::init_sd()
{
    /* Send 80 pulses without cs */
    uint8_t init_buff[8];
    memset(init_buff, 0xFF, sizeof(init_buff));
    if(_spi.write(init_buff, sizeof(init_buff)))
        return RES_SPI_ERR;
    
    return RES_OK;
}

void sd_spi::set_speed(uint32_t speed)
{
    _spi.baud(speed);
}

int8_t sd_spi::send_cmd(cmd_t cmd, uint32_t arg, resp_t resp_type, uint8_t *resp)
{
    int8_t res;
    
    if(cmd != CMD12_STOP_TRANSMISSION)
    {
        res = wait_ready();
        if(res)
            return res;
    }
    
    uint8_t cmd_buff[6];
    cmd_buff[0] = (cmd | (1 << TRANSMITTER_BIT)) & ~R1_START_BIT;
    cmd_buff[1] = arg >> 24;
    cmd_buff[2] = arg >> 16;
    cmd_buff[3] = arg >> 8;
    cmd_buff[4] = arg;
    cmd_buff[5] = calc_crc7(cmd_buff, sizeof(cmd_buff) - 1);
    
    if(_spi.write(cmd_buff, sizeof(cmd_buff)))
        return RES_SPI_ERR;

    uint8_t retry_cnt = WAIT_RESPONSE_CNT;
    while(retry_cnt--)
    {
        resp[0] = 0xFF;
        if(_spi.read(resp, 1))
            return RES_SPI_ERR;
        
        if(!(resp[0] & R1_START_BIT))
            break;
    }
    if(!retry_cnt)
        return RES_NO_RESPONSE;
    
    switch(resp_type)
    {
        case R1:
            break;
        
        case R2:
            res = read_data(&resp[1], 16);
            if(res)
                return res;
            
            // Check the CSD or CID CRC7 (last byte):
            // Raw CID CRC7 always has LSB set to 1,
            //so fix it with CRC7_CID_CSD_MASK
            if(calc_crc7(&resp[1], 15) != (resp[16] & CRC7_CID_CSD_MASK))
                return RES_CRC_ERR;
            break;
        
        case R3:
        case R6:
        case R7:
            memset(&resp[1], 0xFF, 4);
            if(_spi.read(&resp[1], 4))
                return RES_SPI_ERR;
    }
    
    return RES_OK;
}

int8_t sd_spi::read_data(void *data, uint16_t size)
{
    uint8_t data_token;
    uint8_t wait_cnt = 10;
    while(wait_cnt--)
    {
        data_token = 0xFF;
        if(_spi.read(&data_token, 1))
            return RES_SPI_ERR;
        
        if(data_token == DATA_TOKEN_CMD17_18_24)
            break;
        /* Check for error token */
        if(!(data_token & ~ERR_TOKEN_MASK))
        {
            if(data_token & ERR_TOKEN_CARD_LOCKED)
                return RES_LOCKED;
            else if(data_token & ERR_TOKEN_OUT_OF_RANGE_ERR)
                return RES_PARAM_ERR;
            else if(data_token & ERR_TOKEN_ECC_FAIL)
                return RES_READ_ERR;
            else if(data_token & ERR_TOKEN_CC_ERR)
                return RES_READ_ERR;
            else if(data_token & ERR_TOKEN_ERR)
                return RES_READ_ERR;
            else
                return RES_READ_ERR;
        }
        vTaskDelay(1);
    }
    if(!wait_cnt)
        return RES_NO_RESPONSE;
    
    memset(data, 0xFF, size);
    if(_spi.read(data, size))
        return RES_SPI_ERR;
    
    uint8_t crc16[2] = {0xFF, 0xFF};
    if(_spi.read(crc16, sizeof(crc16)))
        return RES_SPI_ERR;
    
    if(calc_crc16((uint8_t *)data, size) != ((crc16[0] << 8) | crc16[1]))
        return RES_CRC_ERR;
    
    return RES_OK;
}

int8_t sd_spi::write_data(void *data, uint16_t size)
{
    if(_spi.write(DATA_TOKEN_CMD17_18_24))
        return RES_SPI_ERR;
    
    if(_spi.write(data, size))
        return RES_SPI_ERR;
    
    uint16_t crc16_tmp = calc_crc16((uint8_t *)data, size);
    uint8_t crc16[2] = {(uint8_t)crc16_tmp, (uint8_t)(crc16_tmp << 8)};
    
    if(_spi.write(crc16, sizeof(crc16)))
        return RES_SPI_ERR;
    
    /* Check data response */
    uint8_t data_resp;
    data_resp = 0xFF;
    if(_spi.read(&data_resp, sizeof(data_resp)))
        return RES_SPI_ERR;
    
    if(!(data_resp & DATA_RESP_START_BIT) || (data_resp & DATA_RESP_END_BIT))
    {
        /* Data response is not valid */
        return RES_WRITE_ERR;
    }
    switch(data_resp & DATA_RESP_MASK)
    {
        case DATA_RESP_ACCEPTED: break;
        case DATA_RESP_CRC_ERR: return RES_CRC_ERR;
        case DATA_RESP_WRITE_ERR:
        default: return RES_WRITE_ERR;
    }
    
    return wait_ready();
}

int8_t sd_spi::wait_ready()
{
    uint8_t busy_flag = 0;
    uint16_t wait_cnt = 500;
    
    while(wait_cnt--)
    {
        busy_flag = 0xFF;
        if(_spi.read(&busy_flag, 1))
            return RES_SPI_ERR;
        
        if(busy_flag == 0xFF)
            break;
        
        vTaskDelay(1);
    };
    
    return wait_cnt ? RES_OK : RES_NO_RESPONSE;
}

static uint8_t calc_crc7(uint8_t *buff, uint8_t size)
{
    uint8_t crc = 0;
    
    while(size--)
        crc = crc7_table[crc ^ *buff++];
    
    return crc;
}

static uint16_t calc_crc16(uint8_t *buff, uint16_t size)
{
    uint16_t crc = 0;
    
    while(size--)
        crc = (crc << 8) ^ crc16_ccitt_table[(crc >> 8) ^ *buff++];
    
    return crc;
}
