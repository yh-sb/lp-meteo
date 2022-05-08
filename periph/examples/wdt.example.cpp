// Example for STM32F4DISCOVERY development board

#include "periph/gpio.hpp"
#include "periph/wdt.hpp"
#include "periph/systick.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace periph;

static void heartbeat_task(void *pvParameters)
{
    gpio *green_led = (gpio *)pvParameters;
    while(1)
    {
        wdt::reload();
        
        green_led->toggle();
        vTaskDelay(500);
    }
}

int main(void)
{
    systick::init();
    static gpio green_led(3, 12, gpio::mode::DO, 0);
    
    wdt::init(1000);
    wdt::on();
    
    xTaskCreate(heartbeat_task, "heartbeat", configMINIMAL_STACK_SIZE,
        &green_led, 1, nullptr);
    
    vTaskStartScheduler();
}
