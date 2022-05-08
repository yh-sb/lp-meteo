// Example for STM32F4DISCOVERY development board

#include "periph/gpio.hpp"
#include "periph/exti.hpp"
#include "periph/systick.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace periph;

static void heartbeat_task(void *pvParameters)
{
    gpio *green_led = (gpio *)pvParameters;
    while(1)
    {
        green_led->toggle();
        vTaskDelay(500);
    }
}

static void exti_cb(exti *exti, void *ctx)
{
    gpio *blue_led = (gpio *)ctx;
    
    blue_led->toggle();
}

int main(void)
{
    systick::init();
    static gpio green_led(3, 12, gpio::mode::DO, 0);
    static gpio blue_led(3, 15, gpio::mode::DO, 0);
    static gpio exti1_gpio(0, 0, gpio::mode::DI, 0);
    
    static exti exti1(exti1_gpio, exti::TRIGGER_FALLING);
    exti1.cb(exti_cb, &blue_led);
    exti1.on();
    
    xTaskCreate(heartbeat_task, "heartbeat", configMINIMAL_STACK_SIZE,
        &green_led, 1, nullptr);
    
    vTaskStartScheduler();
}
