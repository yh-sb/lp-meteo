set(TARGET drv)

add_library(${TARGET} STATIC
    dht/dht.cpp
    di/di.cpp
    dot_matrix/dot_matrix.cpp
    ds18b20/ds18b20.cpp
    gps/nmea.cpp
    onewire/onewire.cpp
    singlewire/singlewire.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        hal
        freertos
        nmea
    PRIVATE
        gpgga gpgll gpgsv gprmc
        printf
)
