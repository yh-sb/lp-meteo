// Example for STM32F072DISCOVERY board

#include <string.h>
#include <assert.h>
#include "periph/gpio.hpp"
#include "periph/dma.hpp"
#include "periph/spi.hpp"
#include "periph/exti.hpp"
#include "periph/tim.hpp"
#include "periph/systick.hpp"
#include "drivers/nrf24l01.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace periph;
using namespace drv;

struct nrf_task_ctx_t
{
    nrf24l01 &nrf;
    gpio &led;
};

static void nrf_task(void *pvParameters)
{
    nrf_task_ctx_t *ctx = (nrf_task_ctx_t *)pvParameters;
    
    int8_t res = ctx->nrf.init();
    assert(res == nrf24l01::RES_OK);
    
    nrf24l01::conf_t conf;
    res = ctx->nrf.get_conf(conf);
    assert(res == nrf24l01::RES_OK);
    
    conf.pipe[1].enable = true;
    conf.pipe[1].addr = 0xA5A5;
    conf.pipe[1].auto_ack = true;
    conf.pipe[1].size = nrf24l01::fifo_size;
    conf.datarate = nrf24l01::datarate::_250_kbps;
    conf.channel = 0;
    conf.retransmit_delay = nrf24l01::ard::_4000_US;
    conf.retransmit_count = 15;
    
    res = ctx->nrf.set_conf(conf);
    assert(res == nrf24l01::RES_OK);
    
    while(1)
    {
        nrf24l01::packet_t packet = {};
        res = ctx->nrf.read(packet);
        if(res == nrf24l01::RES_OK && !strncmp((const char *)packet.buff,
            "Hello!", sizeof("Hello!") - 1))
        {
            ctx->led.toggle();
        }
    }
    res = ctx->nrf.get_conf(conf);
    assert(res == nrf24l01::RES_OK);
    
    conf.pipe[1].enable = false;
    
    res = ctx->nrf.set_conf(conf);
    assert(res == nrf24l01::RES_OK);
}

int main(void)
{
    systick::init();
    static gpio b1(0, 0, gpio::mode::DI, 0);
    static gpio green_led(2, 9, gpio::mode::DO, 0);
    static gpio spi1_mosi_gpio(1, 5, gpio::mode::AF, 1);
    static gpio spi1_miso_gpio(1, 4, gpio::mode::AF, 1);
    static gpio spi1_clk_gpio(1, 3, gpio::mode::AF, 1);
    static gpio nrf24l01_csn(1, 8, gpio::mode::DO, 1);
    static gpio nrf24l01_ce(1, 6, gpio::mode::DO, 0);
    static gpio nrf24l01_irq(1, 7, gpio::mode::DI, 1);
    
    static dma spi1_tx_dma(dma::DMA_1, dma::CH_3, dma::DIR_MEM_TO_PERIPH,
        dma::INC_SIZE_8);
    static dma spi1_rx_dma(dma::DMA_1, dma::CH_2, dma::DIR_PERIPH_TO_MEM,
        dma::INC_SIZE_8);
    
    static spi nrf24l01_spi(spi::SPI_1, 4000000, spi::CPOL_0, spi::CPHA_0,
        spi::BIT_ORDER_MSB, spi1_tx_dma, spi1_rx_dma, spi1_mosi_gpio,
        spi1_miso_gpio, spi1_clk_gpio);
    
    static exti nrf24l01_exti(nrf24l01_irq, exti::TRIGGER_FALLING);
    static tim tim6(tim::TIM_6);
    
    static nrf24l01 nrf(nrf24l01_spi, nrf24l01_csn, nrf24l01_ce, nrf24l01_exti,
        tim6);
    
    static nrf_task_ctx_t nrf_task_ctx = {.nrf = nrf, .led = green_led};
    xTaskCreate(nrf_task, "nrf", configMINIMAL_STACK_SIZE + 20, &nrf_task_ctx,
        1, nullptr);
    
    vTaskStartScheduler();
}
