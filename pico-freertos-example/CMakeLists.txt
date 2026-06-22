cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico CACHE STRING "Target board")
set(PICO_SDK_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../pico-sdk
    CACHE PATH "Path to the Pico SDK" FORCE)

include(${CMAKE_CURRENT_LIST_DIR}/../pico-sdk/external/pico_sdk_import.cmake)

project(pico_freertos_example C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(FREERTOS_KERNEL_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../FreeRTOS-Kernel
    CACHE PATH "Path to the FreeRTOS Kernel")
set(FREERTOS_CONFIG_FILE_DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE PATH "Path to FreeRTOSConfig.h")

set(WIFI_SSID "" CACHE STRING "Wi-Fi SSID for the Pico 2 W web dashboard")
set(WIFI_PASSWORD "" CACHE STRING "Wi-Fi password for the Pico 2 W web dashboard")
set(WIFI_AUTH "CYW43_AUTH_WPA2_AES_PSK" CACHE STRING "CYW43 Wi-Fi auth mode")

include(
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

add_executable(pico_freertos_example
    app_state.c
    dht11.c
    dip_switch.c
    main.c
    switch_sound.c
    tm1637.c
    web_server.c
)

target_include_directories(pico_freertos_example PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(pico_freertos_example PRIVATE
    pico_stdlib
    hardware_clocks
    hardware_pwm
    FreeRTOS-Kernel-Heap4
)

target_compile_definitions(pico_freertos_example PRIVATE
    WIFI_SSID="${WIFI_SSID}"
    WIFI_PASSWORD="${WIFI_PASSWORD}"
    WIFI_AUTH=${WIFI_AUTH}
)

if (TARGET pico_cyw43_arch_lwip_sys_freertos)
    target_link_libraries(pico_freertos_example PRIVATE
        pico_cyw43_arch_lwip_sys_freertos
    )
    target_compile_definitions(pico_freertos_example PRIVATE
        ENABLE_WIFI_WEB=1
    )
else()
    target_compile_definitions(pico_freertos_example PRIVATE
        ENABLE_WIFI_WEB=0
    )
endif()

pico_enable_stdio_usb(pico_freertos_example 1)
pico_enable_stdio_uart(pico_freertos_example 1)
pico_add_extra_outputs(pico_freertos_example)
