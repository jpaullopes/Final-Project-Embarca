# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(alertaTemperatura C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Inicialize variáveis de inclusões comuns para todos os targets
set(COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${PICO_SDK_PATH}/src/common/pico_stdlib/include
    ${PICO_SDK_PATH}/src/common/pico_base/include
    ${PICO_SDK_PATH}/src/common/pico_time/include
    ${PICO_SDK_PATH}/src/common/pico_sync/include
    ${PICO_SDK_PATH}/src/common/pico_util/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_i2c/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_clocks/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_platform/include
    ${PICO_SDK_PATH}/src/rp2040/hardware_structs/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_base/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
)

add_library(tcp_client STATIC
    ${CMAKE_CURRENT_LIST_DIR}/src/tcp_client.c
    ${CMAKE_CURRENT_LIST_DIR}/include/tcp_client.h
)

# Add include directories for tcp_client
target_include_directories(tcp_client PUBLIC ${COMMON_INCLUDE_DIRS})

# Link libraries for tcp_client
target_link_libraries(tcp_client
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
)

add_executable(alertaTemperatura alertaTemperatura.c include/oled/ssd1306_i2c.c include/ledsArray.h)

pico_set_program_name(alertaTemperatura "alertaTemperatura")
pico_set_program_version(alertaTemperatura "0.1")

pico_generate_pio_header(alertaTemperatura ${CMAKE_CURRENT_LIST_DIR}/pio/ws2818b.pio)

target_link_libraries(alertaTemperatura 
    pico_stdlib
    hardware_i2c
    hardware_adc
    hardware_pwm
    hardware_clocks
    hardware_pio
    hardware_dma
    hardware_spi
    hardware_timer
    hardware_uart
    hardware_watchdog
    pico_stdio
    pico_cyw43_arch_lwip_threadsafe_background
    tcp_client
)

# Modificar as linhas abaixo para habilitar/desabilitar saída através de UART/USB
pico_enable_stdio_uart(alertaTemperatura 0)
pico_enable_stdio_usb(alertaTemperatura 1)

# Adicionar os diretórios include padrão ao build
target_include_directories(alertaTemperatura PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/include/oled
)

pico_add_extra_outputs(alertaTemperatura)

