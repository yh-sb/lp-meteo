add_library(freertos_wrappers INTERFACE)

target_include_directories(freertos_wrappers
    INTERFACE
        .
)

target_link_libraries(freertos_wrappers
    INTERFACE
        freertos_kernel
)
