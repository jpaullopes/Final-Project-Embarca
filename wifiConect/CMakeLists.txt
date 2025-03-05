cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(wifiConect C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(wifiConect wifiConect.c)

pico_set_program_name(wifiConect "wifiConect")
pico_set_program_version(wifiConect "0.1")

# Generate PIO header (se precisar, senão remova)
# pico_generate_pio_header(wifiConect ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify stdio settings
pico_enable_stdio_uart(wifiConect 0)
pico_enable_stdio_usb(wifiConect 1)

# Link libraries
target_link_libraries(wifiConect
        pico_stdlib
        pico_w
        cyw43_arch
        pico_lwip
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        #hardware_arpanet # REMOVIDA
        pico_cyw43_arch_lwip_poll # REMOVIDA inicialmente
)

# **ADICIONANDO DIRETÓRIOS DE INCLUDE DO SDK:**
target_include_directories(wifiConect PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
        ${PICO_SDK_PATH}/lib/cyw43-driver/src
        ${PICO_SDK_PATH}/lib/lwip/src/include
)


pico_add_extra_outputs(wifiConect)