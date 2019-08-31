#include "common/assert.h"
#include "gpio/gpio.hpp"
#include "uart/uart.hpp"
#include "tim/tim.hpp"
#include "exti/exti.hpp"
#include "rtc/rtc.hpp"
#include "wdt/wdt.hpp"
#include "drv/singlewire/singlewire.hpp"
#include "drv/dht/dht.hpp"
#include "drv/onewire/onewire.hpp"
#include "drv/ds18b20/ds18b20.hpp"
#include "drv/gps/nmea.hpp"
#include "task/dht11.hpp"
#include "task/ds18b20.hpp"
#include "task/gps.hpp"
#include "task/ui.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace hal;

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
	
	static gpio green_led(3, 12, gpio::MODE_DO, 0);
	static gpio dht11_gpio(2, 5, gpio::MODE_OD, 1);
	static gpio dht11_exti_gpio(0, 7, gpio::MODE_DI, 1);
	static gpio uart2_tx_gpio(0, 2, gpio::MODE_AF, 0);
	static gpio uart2_rx_gpio(0, 3, gpio::MODE_AF, 0);
	static gpio uart3_tx_gpio(3, 8, gpio::MODE_AF, 0);
	static gpio uart3_rx_gpio(1, 11, gpio::MODE_AF, 0);
	
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
	
	static drv::nmea gps(uart3, tskIDLE_PRIORITY + 1);
	
	QueueHandle_t ui_queue = xQueueCreate(1, sizeof(task::ui_queue_t));
	ASSERT(ui_queue);
	
	static const drv::di *di_ctx[] = {&b1_di, &cd_di};
	
	static task::dht11_ctx_t dht11_ctx =
		{.to_ui = ui_queue, .dht11 = &dht11};
	
	static task::ds18b20_ctx_t ds18b20_ctx =
		{.to_ui = ui_queue, ._ds18b20 = &_ds18b20};
	
	static task::gps_ctx_t gps_ctx = {.nmea = &gps};
	
	static task::ui_ctx_t ui_ctx = {.to_ui = ui_queue};
	
	ASSERT(xTaskCreate(safety_task, "safety", configMINIMAL_STACK_SIZE * 1,
		&green_led, tskIDLE_PRIORITY + 5, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(task::dht11, "dht11", configMINIMAL_STACK_SIZE * 3,
		&dht11_ctx, tskIDLE_PRIORITY + 4, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(task::ds18b20, "ds18b20", configMINIMAL_STACK_SIZE * 3,
		&ds18b20_ctx, tskIDLE_PRIORITY + 3, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(task::gps, "gps", configMINIMAL_STACK_SIZE * 11,
		&gps_ctx, tskIDLE_PRIORITY + 2, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(task::ui, "ui", configMINIMAL_STACK_SIZE * 9,
		&ui_ctx, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(task::ui, "ui", configMINIMAL_STACK_SIZE * 8,
		&ui_ctx, tskIDLE_PRIORITY + 3, NULL) == pdPASS);
	
	vTaskStartScheduler();
}
