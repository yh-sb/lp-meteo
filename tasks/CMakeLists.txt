add_library(tasks STATIC
    src/dht11.cpp
    src/ds18b20.cpp
    src/gps.cpp
    src/ui.cpp
)

target_include_directories(tasks
    PUBLIC
        include
)

target_link_libraries(tasks
    PUBLIC
        drivers
        freertos_kernel
    PRIVATE
        periph
        syslog
)
