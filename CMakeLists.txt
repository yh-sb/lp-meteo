cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE cmake/toolchain-arm-gcc-cortex.cmake)

project(lp-meteo C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(
    -mcpu=cortex-m4
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -mthumb
    -ffunction-sections
    -fdata-sections
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
)

add_subdirectory(drivers)
add_subdirectory(periph)
add_subdirectory(tasks)

set(DEVICE_NAME STM32F407xx)
add_subdirectory(third_party/CMSIS)
#add_subdirectory(third_party/CMSIS/DSP)

set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")
set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} CACHE STRING "")
add_subdirectory(third_party/FreeRTOS-Kernel)

set(NMEA_BUILD_SHARED_LIB OFF CACHE BOOL "")
set(NMEA_BUILD_EXAMPLES OFF CACHE BOOL "")
set(NMEA_UNIT_TESTS OFF CACHE BOOL "")
add_subdirectory(third_party/libnmea)

add_subdirectory(third_party/printf)

add_subdirectory(utilities/freertos_wrappers)
add_subdirectory(utilities/syslog)

add_executable(${CMAKE_PROJECT_NAME} main.cpp assert.c fault_handlers.c FreeRTOSHooks.c)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    cmsis
    #cmsis-dsp
    drivers
    periph
    tasks
    freertos_kernel
    printf
    syslog
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -mcpu=cortex-m4
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -Wl,--gc-sections
    -specs=nano.specs
    -specs=nosys.specs
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map,--cref
    -T ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CMSIS/Device/ST/STM32F4xx/Source/linker/STM32F407VGTx_FLASH.ld
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} ${CMAKE_PROJECT_NAME}.elf
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf "${CMAKE_PROJECT_NAME}.bin"
)

# Add targets for flashing, erasing, resetting and debugging
set(JLINK_PARAMS -device STM32F407VG -if SWD)
include(cmake/debug-probes/jlink.cmake)
