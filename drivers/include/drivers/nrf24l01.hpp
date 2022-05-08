#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "periph/gpio.hpp"
#include "periph/spi.hpp"
#include "periph/exti.hpp"
#include "periph/tim.hpp"
#include "FreeRTOS.h"
#include "semphr.h"

namespace drv
{
class nrf24l01
{
public:
    enum res_t
    {
        RES_OK                  =  0,
        RES_SPI_ERR             = -1,
        RES_TX_MAX_RETRIES_ERR  = -2,
        RES_NRF_NO_RESPONSE_ERR = -3
    };
    
    enum class dev {NRF24L01, NRF24L01_PLUS};
    
    static constexpr auto pipes = 6;
    static constexpr auto fifo_size = 32;
    
    nrf24l01(periph::spi &spi, periph::gpio &cs, periph::gpio &ce,
        periph::exti &exti, periph::tim &tim, dev dev = dev::NRF24L01_PLUS);
    ~nrf24l01();
    
    int8_t init();
    
    // 250 kbps datarate available only for nrf24l01+
    enum class datarate : uint8_t {_250_kbps, _1_Mbps, _2_Mbps};
    enum class pwr : uint8_t {_0_dBm, _6_dBm, _12_dBm, _18_dBm};
    enum class ard : uint8_t
    {
        _250_US, _500_US, _750_US, _1000_US, _1250_US, _1500_US, _1750_US,
        _2000_US, _2250_US, _2500_US, _2750_US, _3000_US, _3250_US,
        _3500_US, _3750_US, _4000_US
    };
#pragma pack(push, 1)
    struct conf_t
    {
        /* Configuration struct for rx pipes. NOTE: pipe 0 is used in tx
        setup to receive auto ack. So, don't use pipe 0 for reading if tx
        functionality will be used */
        struct
        {
            // Enable rx pipe
            bool enable:1;
            // Size of pipe payload. Could be ignored if dyn_payload=true
            uint8_t size:6;
            /* Pipe address. Pipes 0,1 have 5-byte address, 2-5 have 1-byte
            address (LSByte) */
            uint64_t addr:40;
            // Auto acknowledgement for received packets for specific pipe
            bool auto_ack:1;
            // Dynamic payload size. Also requires auto_ack enabled
            bool dyn_payload:1;
        } pipe[pipes];
        /* 5-byte tx address. If auto ack is enabled, pipe 0 should has the
        same address */
        uint64_t tx_addr:40;
        /* Auto acknowledgement for transmitted packets.
        NOTE: Pipe 0 will be also configured for auto ack */
        bool tx_auto_ack:1;
        /* Dynamic payload size feature. Should be enabled for tx or rx
        dynamic payload size support */
        bool dyn_payload:1;
        enum datarate datarate;
        enum pwr power;
        // RF channel. Possible range is 0 - 125 (2400 to 2525 MHz)
        uint8_t channel;
        ard retransmit_delay;
        uint8_t retransmit_count:4;
    };
#pragma pack(pop)
    int8_t get_conf(conf_t &conf);
    int8_t set_conf(conf_t &conf);
    
    struct packet_t
    {
        uint8_t pipe;
        uint8_t buff[fifo_size];
        uint8_t size;
    };
    /**
     * @brief Read packet.
     * @note This method changes nrf24l01 mode from power-down to rx.
     * 
     * @param packet Packet to receive.
     * @param ack Ack payload to be transmitted after reception. Don't
     * forget to specify ack size (ack->size) and pipe index (ack->pipe) for
     * which this ack will be transmitted. dyn_payload should be also
     * enabled for specific pipes.
     * @return int8_t Error code. @see @ref res_t
     */
    int8_t read(packet_t &packet, packet_t *ack = NULL);
    
    /**
     * @brief Write data buffer.
     * @note This method changes nrf24l01 mode from power-down to tx.
     * 
     * @param buff Pointer to data buffer.
     * @param size Size of data buffer. It must match the payload size on
     * receiving side or use dynamic payload size.
     * @param ack Ack payload to be received after transmission.
     * @param is_continuous_tx Is further continuous transmission planned.
     * In case of continuous writing, tx mode will be enabled all the time
     * to avoid redundant 130 us (standby-1 mode -> tx mode) delay.
     * @return int8_t Error code. @see @ref res_t
     */
    int8_t write(void *buff, size_t size, packet_t *ack = NULL,
        bool is_continuous_tx = false);
    
    /**
     * @brief Go to power down mode.
     * Usually this method shouldn't be used in common workflow. Power down
     * mode is managed automatically by this driver.
     * It might by used after write() method when no more future tx/rx
     * actions is planned.
     * Power down mode is automatically activated when none of the rx pipes
     * are opened.
     * 
     * @return int8_t Error code. @see @ref res_t
     */
    int8_t power_down();
    
private:
    enum class reg : uint8_t
    {
        CONFIG,
        EN_AA,
        EN_RXADDR,
        SETUP_AW,
        SETUP_RETR,
        RF_CH,
        RF_SETUP,
        STATUS,
        OBSERVE_TX,
        RPD, // Named "CD (Carrier Detect)" in nrf24l01 documentation
        RX_ADDR_P0,
        RX_ADDR_P1,
        RX_ADDR_P2,
        RX_ADDR_P3,
        RX_ADDR_P4,
        RX_ADDR_P5,
        TX_ADDR,
        RX_PW_P0,
        RX_PW_P1,
        RX_PW_P2,
        RX_PW_P3,
        RX_PW_P4,
        RX_PW_P5,
        FIFO_STATUS,
        DYNPD = 28, // Only for nrf24l01+
        FEATURE = 29 // Only for nrf24l01+
    };
    int8_t read_reg(reg reg, void *data, size_t size = 1);
    int8_t write_reg(reg reg, void *data, size_t size = 1);
    
    enum class instruction : uint8_t
    {
        R_REGISTER         = 0x00,
        W_REGISTER         = 0x20,
        R_RX_PAYLOAD       = 0x61,
        W_TX_PAYLOAD       = 0xA0,
        FLUSH_TX           = 0xE1,
        FLUSH_RX           = 0xE2,
        REUSE_TX_PL        = 0xE3,
        ACTIVATE           = 0x50, // Only for nrf24l01+
        R_RX_PL_WID        = 0x60, // Only for nrf24l01+
        W_ACK_PAYLOAD      = 0xA8, // Only for nrf24l01+
        W_TX_PAYLOAD_NOACK = 0xB0, // Only for nrf24l01+
        NOP                = 0xFF
    };
    int8_t exec_instruction(instruction instruction);
    int8_t exec_instruction_with_read(instruction instruction, void *buff,
        size_t size);
    int8_t exec_instruction_with_write(instruction instruction, void *buff,
        size_t size);
    
    enum class mode { PWR_DOWN, STANDBY_1, TX, RX };
    int8_t set_mode(mode mode);
    
    bool is_conf_valid(conf_t &conf);
    int8_t ack_payload(bool enable);
    
    void delay(uint32_t us);
    
    static void exti_cb(periph::exti *exti, void *ctx);
    static void tim_cb(periph::tim *tim, void *ctx);
    
    periph::spi &_spi;
    periph::gpio &_cs;
    periph::gpio &_ce;
    periph::exti &_exti;
    periph::tim &_tim;
    SemaphoreHandle_t api_lock;
    
    struct
    {
        enum dev dev;
        enum mode mode;
        struct
        {
            uint8_t size;
            bool dyn_payload;
        } pipe[pipes];
        bool ack_payload;
        bool dyn_payload;
    } _conf;
};
}
