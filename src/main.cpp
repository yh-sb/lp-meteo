#include "common/assert.h"
#include "gpio/gpio.hpp"
#include "uart/uart.hpp"
#include "tim/tim.hpp"
#include "exti/exti.hpp"
#include "rtc/rtc.hpp"
#include "spi/spi.hpp"
#include "wdt/wdt.hpp"
#include "drv/singlewire/singlewire.hpp"
#include "drv/dht/dht.hpp"
#include "drv/di/di.hpp"
#include "drv/onewire/onewire.hpp"
#include "drv/ds18b20/ds18b20.hpp"
#include "drv/hd44780/hd44780.hpp"
#include "drv/gps/nmea.hpp"
#include "drv/sd/sd_spi.hpp"
#include "ul/fatfs_diskio/fatfs_diskio.hpp"
#include "third_party/FatFs/ff.h"
#include "task/dht11.hpp"
#include "task/ds18b20.hpp"
#include "task/gps.hpp"
#include "task/ui.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace hal;

static void b1_cb(drv::di *di, bool state, void *ctx);
static void cd_cb(drv::di *di, bool state, void *ctx);

static void di_task(void *pvParameters)
{
	drv::di *b1 = ((drv::di **)pvParameters)[0];
	drv::di *cd = ((drv::di **)pvParameters)[1];
	
	while(1)
	{
		b1->poll();
		cd->poll();
		vTaskDelay(1);
	}
}

static void safety_task(void *pvParameters)
{
	gpio *green_led = (gpio *)pvParameters;
	while(1)
	{
		green_led->toggle();
		//wdt::reload();
		vTaskDelay(500);
	}
}

int main(void)
{
	//wdt::init(1000);
	//wdt::on();
	
	rtc::init(rtc::CLK_LSI);
	struct tm time = {};
	time.tm_year = 119;
	time.tm_mday = 1;
	rtc::set(time);
	
	static gpio b1(0, 0, gpio::MODE_DI, 0);
	static gpio green_led(3, 12, gpio::MODE_DO, 0);
	static gpio dht11_gpio(2, 5, gpio::MODE_OD, 1);
	static gpio dht11_exti_gpio(0, 7, gpio::MODE_DI, 1);
	static gpio uart2_tx_gpio(0, 2, gpio::MODE_AF, 0);
	static gpio uart2_rx_gpio(0, 3, gpio::MODE_AF, 0);
	static gpio uart3_tx_gpio(3, 8, gpio::MODE_AF, 0);
	static gpio uart3_rx_gpio(1, 11, gpio::MODE_AF, 0);
	static gpio rs(0, 5, gpio::MODE_DO);
	static gpio rw(0, 4, gpio::MODE_DO);
	static gpio e(1, 1, gpio::MODE_DO);
	static gpio db4(0, 6, gpio::MODE_DO);
	static gpio db5(1, 4, gpio::MODE_DO);
	static gpio db6(0, 8, gpio::MODE_DO);
	static gpio db7(1, 5, gpio::MODE_DO);
	static gpio spi2_mosi(1, 15, gpio::MODE_AF, 0);
	static gpio spi2_miso(2, 2, gpio::MODE_AF, 0);
	static gpio spi2_clk(1, 13, gpio::MODE_AF, 0);
	static gpio sd_cs(1, 12, gpio::MODE_DO, 1);
	static gpio sd_cd(1, 14, gpio::MODE_DI, 1);
	
	static tim hd44780_tim(tim::TIM_6);
	static tim dht11_tim(tim::TIM_7);
	
	static exti dht11_exti(dht11_exti_gpio);
	
	static drv::hd44780 lcd(rs, rw, e, db4, db5, db6, db7, hd44780_tim);
	
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
	
	static dma spi2_rx_dma(dma::DMA_2, dma::STREAM_3, dma::CH_0,
		dma::DIR_PERIPH_TO_MEM, dma::INC_SIZE_8);
	static dma spi2_tx_dma(dma::DMA_1, dma::STREAM_4, dma::CH_0,
		dma::DIR_MEM_TO_PERIPH, dma::INC_SIZE_8);
	
	static uart uart2(uart::UART_2, 115200, uart::STOPBIT_1, uart::PARITY_NONE,
		uart2_tx_dma, uart2_rx_dma, uart2_tx_gpio, uart2_rx_gpio);
	static uart uart3(uart::UART_3, 9600, uart::STOPBIT_1, uart::PARITY_NONE,
		uart3_tx_dma, uart3_rx_dma, uart3_tx_gpio, uart3_rx_gpio);
	
	static spi spi2(spi::SPI_2, 1000000, spi::CPOL_0, spi::CPHA_0,
		spi::BIT_ORDER_MSB, spi2_tx_dma, spi2_rx_dma, spi2_mosi, spi2_miso,
		spi2_clk);
	
	static drv::sd_spi sd1(spi2, sd_cs, &sd_cd);
	
	static drv::di b1_di(b1, 30, 1);
	b1_di.cb(b1_cb, NULL);
	static drv::di cd_di(sd_cd, 100, 1);
	cd_di.cb(cd_cb, NULL);
	
	static drv::onewire _onewire(uart2);
	static drv::ds18b20 _ds18b20(_onewire);
	
	static drv::nmea gps(uart3, tskIDLE_PRIORITY + 1);
	
	ul::fatfs_diskio_add(0, sd1);
	
	QueueHandle_t ui_queue = xQueueCreate(1, sizeof(task::ui_queue_t));
	ASSERT(ui_queue);
	
	static const drv::di *di_ctx[] = {&b1_di, &cd_di};
	
	static task::dht11_ctx_t dht11_ctx =
		{.to_ui = ui_queue, .dht11 = &dht11};
	
	static task::ds18b20_ctx_t ds18b20_ctx =
		{.to_ui = ui_queue, ._ds18b20 = &_ds18b20};
	
	static task::gps_ctx_t gps_ctx =
		{.nmea = &gps, .lcd = &lcd};
	
	static task::ui_ctx_t ui_ctx =
		{.to_ui = ui_queue, .lcd = &lcd};
	
	ASSERT(xTaskCreate(safety_task, "safety", configMINIMAL_STACK_SIZE * 1,
		&green_led, tskIDLE_PRIORITY + 5, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(di_task, "di", configMINIMAL_STACK_SIZE * 10,
		di_ctx, tskIDLE_PRIORITY + 4, NULL));
	
	ASSERT(xTaskCreate(task::dht11, "dht11", configMINIMAL_STACK_SIZE * 2,
		&dht11_ctx, tskIDLE_PRIORITY + 2, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(task::ds18b20, "ds18b20", configMINIMAL_STACK_SIZE * 2,
		&ds18b20_ctx, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(task::gps, "gps", configMINIMAL_STACK_SIZE * 10,
		&gps_ctx, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
	
	ASSERT(xTaskCreate(task::ui, "ui", configMINIMAL_STACK_SIZE * 8,
		&ui_ctx, tskIDLE_PRIORITY + 3, NULL) == pdPASS);
	
	vTaskStartScheduler();
}

static void b1_cb(drv::di *di, bool state, void *ctx)
{
	if(!state)
		return;
	
	/* static bool led_state = true;
	hal::gpio *led = (hal::gpio *)ctx;
	
	led_state = !led_state;
	led->set(!led_state);
	
	log().inf("Button b1 was pressed");
	log().inf("Light LED is %s", led_state ? "ON" : "OFF");*/
}

static void cd_cb(drv::di *di, bool state, void *ctx)
{
	if(state)
	{
		f_unmount("SD");
		return;
	}
	
	FATFS fs;
	FRESULT ff_res = f_mount(&fs, "SD", 1);
	if(ff_res)
	{
		BYTE work_area[FF_MAX_SS] = {};
		ff_res = f_mkfs("SD", FM_FAT32, 0, work_area, sizeof(work_area));
		if(ff_res)
			return;
		ff_res = f_mount(&fs, "SD", 1);
		if(ff_res)
			return;
	}
	
	FIL file;
	ff_res = f_open(&file, "SD:test.txt", FA_WRITE | FA_CREATE_ALWAYS);
	if(ff_res)
		return;
	
	size_t size = strlen("abcdefgh-1234567890");
	ff_res = f_write(&file, "abcdefgh-1234567890", size, &size);
	f_close(&file);
	if(ff_res)
		return;
	
	ff_res = f_open(&file, "SD:test.txt", FA_READ);
	if(ff_res)
		return;
	
	char buff[64] = {};
	size = 0;
	ff_res = f_read(&file, buff, sizeof(buff), &size);
	f_close(&file);
}
