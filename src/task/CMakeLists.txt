set(TARGET task)

add_library(${TARGET} STATIC
    dht11.cpp
    ds18b20.cpp
    gps.cpp
    ui.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        drv
        freertos
    PRIVATE
        hal
        syslog
)
