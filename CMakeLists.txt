cmake_minimum_required(VERSION 3.13)

# Set Pico 2 W board BEFORE including SDK
set(PICO_BOARD pico2_w CACHE STRING "Board type")

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(hydroponic_controller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Export compile commands for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_executable(hydroponic_controller
    src/main.cpp
    src/hydroponic_controller.cpp
    src/config.cpp
    
    # Utils
    src/utils/time_utils.cpp
    src/utils/gpio_utils.cpp
    
    # Sensor libraries
    lib/pico_onewire/onewire_pio.cpp
    lib/pico_onewire/ds18b20.cpp
    lib/pico_sht30/sht30.cpp
    lib/pico_dht22/dht22.cpp
    lib/pico_nrf24l01/nrf24l01.cpp
    lib/pico_nrf24l01/nano_nrf_receiver.cpp
    
    # Application sensors
    src/sensors/sensor_manager.cpp
    
    # Control
    src/control/lights_controller.cpp
    src/control/pump_controller.cpp
    src/control/heater_controller.cpp
    src/control/fan_controller.cpp
    
    # Network
    src/network/network_manager.cpp
    src/network/tcp_server.cpp
    src/network/web_server.cpp
    
    # Storage
    src/storage/flash_storage.cpp
)

target_include_directories(hydroponic_controller PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/lib/lwip/src/include
    ${CMAKE_CURRENT_LIST_DIR}/lib/littlefs
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_onewire
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_dht22
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_sht30
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_nrf24l01
)

# Add preprocessor definition to handle missing pico/rand.h
target_compile_definitions(hydroponic_controller PRIVATE LWIP_RAND_FUNCTION=rand)

target_link_libraries(hydroponic_controller 
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_multicore
    hardware_i2c
    hardware_spi
    hardware_gpio
    hardware_pwm
    hardware_flash
    hardware_sync
    hardware_pio
    cyw43_driver
)

# Add LittleFS source files
target_sources(hydroponic_controller PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/lib/littlefs/lfs.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/littlefs/lfs_util.c
)

# Add SNTP source file
target_sources(hydroponic_controller PRIVATE
    /usr/share/pico-sdk/lib/lwip/src/apps/sntp/sntp.c
)

# Add PIO program
pico_generate_pio_header(hydroponic_controller ${CMAKE_CURRENT_LIST_DIR}/lib/pico_onewire/onewire.pio)

pico_enable_stdio_usb(hydroponic_controller 1)
pico_enable_stdio_uart(hydroponic_controller 0)

pico_add_extra_outputs(hydroponic_controller)

