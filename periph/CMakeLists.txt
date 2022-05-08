add_library(periph STATIC
    src/dac.cpp
    src/dma.cpp
    src/exti.cpp
    src/gpio.cpp
    src/i2c.cpp
    src/pwm.cpp
    src/rcc.cpp
    src/rtc.cpp
    src/spi.cpp
    src/systick.cpp
    src/tim.cpp
    src/uart.cpp
    src/wdt.cpp
)

target_include_directories(periph
    PUBLIC
        include
)

target_link_libraries(periph
    PUBLIC
        freertos_kernel
    PRIVATE
        cmsis
)
