#include <assert.h>
#include "periph/gpio.hpp"
#include "periph/uart.hpp"
#include "periph/tim.hpp"
#include "periph/exti.hpp"
#include "periph/rtc.hpp"
#include "periph/wdt.hpp"
#include "drivers/singlewire.hpp"
#include "drivers/dht.hpp"
#include "drivers/onewire.hpp"
#include "drivers/ds18b20.hpp"
#include "drivers/dot_matrix.hpp"
#include "drivers/nmea_reader.hpp"
#include "tasks/dht11.hpp"
#include "tasks/ds18b20.hpp"
#include "tasks/gps.hpp"
#include "tasks/ui.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "dot_matrix_font.cpp"

using namespace periph;

static void safety_task(void *pvParameters)
{
    gpio *green_led = (gpio *)pvParameters;
    while(1)
    {
        green_led->toggle();
        wdt::reload();
        vTaskDelay(500);
    }
}

int main(void)
{
    wdt::init(1000);
    wdt::on();
    
    rtc::init(rtc::CLK_LSI);
    struct tm time = {};
    time.tm_year = 119;
    time.tm_mday = 1;
    rtc::set(time);
    
    static gpio green_led(3, 12, gpio::mode::DO, 0);
    static gpio dht11_gpio(2, 5, gpio::mode::OD, 1);
    static gpio dht11_exti_gpio(0, 7, gpio::mode::DI, 1);
    static gpio uart2_tx_gpio(0, 2, gpio::mode::AF, 0);
    static gpio uart2_rx_gpio(0, 3, gpio::mode::AF, 0);
    static gpio uart3_tx_gpio(3, 8, gpio::mode::AF, 0);
    static gpio uart3_rx_gpio(1, 11, gpio::mode::AF, 0);
    static gpio shift_reg_bit0(4, 10, gpio::mode::DO, 1);
    static gpio shift_reg_bit1(4, 9, gpio::mode::DO, 1);
    static gpio shift_reg_bit2(4, 7, gpio::mode::DO, 1);
    static gpio shift_reg_bit3(4, 8, gpio::mode::DO, 1);
    static gpio shift_reg_bit4(4, 13, gpio::mode::DO, 1);
    static gpio shift_reg_bit5(4, 14, gpio::mode::DO, 1);
    static gpio shift_reg_bit6(4, 12, gpio::mode::DO, 1);
    static gpio shift_reg_bit7(4, 11, gpio::mode::DO, 1);
    static gpio shift_reg_clk(4, 15, gpio::mode::DO, 1);
    
    static tim dht11_tim(tim::TIM_7);
    
    static exti dht11_exti(dht11_exti_gpio);
    
    static drv::singlewire dht11_singlewire(dht11_gpio, dht11_tim, dht11_exti);
    static drv::dht dht11(dht11_singlewire, drv::dht::DHT11);
    
    static dma uart2_tx_dma(dma::dma_t::DMA_1, dma::stream_t::STREAM_6,
        dma::ch_t::CH_4, dma::dir_t::DIR_MEM_TO_PERIPH, dma::inc_size_t::INC_SIZE_8);
    static dma uart2_rx_dma(dma::dma_t::DMA_1, dma::stream_t::STREAM_5,
        dma::ch_t::CH_4, dma::dir_t::DIR_PERIPH_TO_MEM, dma::inc_size_t::INC_SIZE_8);
    static dma uart3_tx_dma(dma::dma_t::DMA_1, dma::stream_t::STREAM_3,
        dma::ch_t::CH_4, dma::dir_t::DIR_MEM_TO_PERIPH, dma::inc_size_t::INC_SIZE_8);
    static dma uart3_rx_dma(dma::dma_t::DMA_1, dma::stream_t::STREAM_1,
        dma::ch_t::CH_4, dma::dir_t::DIR_PERIPH_TO_MEM, dma::inc_size_t::INC_SIZE_8);
    
    static uart uart2(uart::UART_2, 115200, uart::STOPBIT_1, uart::PARITY_NONE,
        uart2_tx_dma, uart2_rx_dma, uart2_tx_gpio, uart2_rx_gpio);
    static uart uart3(uart::UART_3, 9600, uart::STOPBIT_1, uart::PARITY_NONE,
        uart3_tx_dma, uart3_rx_dma, uart3_tx_gpio, uart3_rx_gpio);
    
    static drv::onewire _onewire(uart2);
    static drv::ds18b20 _ds18b20(_onewire);
    
    static drv::nmea_reader gps(uart3, tskIDLE_PRIORITY + 1);
    
    gpio *shift_reg_bits[] =
    {
        &shift_reg_bit0, &shift_reg_bit1, &shift_reg_bit2,
        &shift_reg_bit3, &shift_reg_bit4, &shift_reg_bit5,
        &shift_reg_bit6, &shift_reg_bit7
    };
    static drv::dot_matrix matrix(8, 80, shift_reg_bits, shift_reg_clk, true);
    
    matrix.load_symbols(dot_matrix_font, sizeof(dot_matrix_font) /
        sizeof(dot_matrix_font[0]));
    
    QueueHandle_t ui_queue = xQueueCreate(2, sizeof(tasks::ui_queue_t));
    assert(ui_queue);
    
    assert(xTaskCreate(safety_task, "safety", configMINIMAL_STACK_SIZE,
        &green_led, tskIDLE_PRIORITY + 5, nullptr) == pdPASS);
    
    static tasks::dht11_ctx_t dht11_ctx = { .to_ui = ui_queue, .dht11 = &dht11 };
    assert(xTaskCreate(tasks::dht11, "dht11", configMINIMAL_STACK_SIZE,
        &dht11_ctx, tskIDLE_PRIORITY + 4, nullptr) == pdPASS);
    
    static tasks::ds18b20_ctx_t ds18b20_ctx = { .to_ui = ui_queue, ._ds18b20 = &_ds18b20 };
    assert(xTaskCreate(tasks::ds18b20, "ds18b20", configMINIMAL_STACK_SIZE,
        &ds18b20_ctx, tskIDLE_PRIORITY + 3, nullptr) == pdPASS);
    
    static tasks::gps_ctx_t gps_ctx = { .nmea = &gps };
    assert(xTaskCreate(tasks::gps, "gps", configMINIMAL_STACK_SIZE + 60,
        &gps_ctx, tskIDLE_PRIORITY + 2, nullptr) == pdPASS);
    
    static tasks::ui_ctx_t ui_ctx = { .to_ui = ui_queue, .matrix = &matrix };
    assert(xTaskCreate(tasks::ui, "ui", configMINIMAL_STACK_SIZE + 50,
        &ui_ctx, tskIDLE_PRIORITY + 1, nullptr) == pdPASS);
    
    vTaskStartScheduler();
}
