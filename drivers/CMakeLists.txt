add_library(drivers STATIC
    src/dataflash.cpp
    src/dht.cpp
    src/di.cpp
    src/dot_matrix.cpp
    src/ds18b20.cpp
    src/enc.cpp
    src/hd44780.cpp
    src/nmea_reader.cpp
    src/nrf24l01.cpp
    src/onewire.cpp
    src/sd.cpp
    src/sd_spi.cpp
    src/singlewire.cpp
)

target_include_directories(drivers
    PUBLIC
        include
)

target_link_libraries(drivers
    PUBLIC
        periph
        freertos_kernel
        nmea
    PRIVATE
        printf
        freertos_wrappers
)
