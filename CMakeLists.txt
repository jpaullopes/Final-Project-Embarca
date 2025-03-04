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
set(sdkVersion 1.5.1)
set(toolchainVersion 13_3_Rel1)
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

# Add executable. Default name is the project name, version 0.1
add_executable(alertaTemperatura alertaTemperatura.c include/oled/ssd1306_i2c.c include/ledsArray.h)

pico_set_program_name(alertaTemperatura "alertaTemperatura")
pico_set_program_version(alertaTemperatura "0.1")

pico_generate_pio_header(alertaTemperatura ${CMAKE_CURRENT_LIST_DIR}/pio/ws2818b.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(alertaTemperatura 0)
pico_enable_stdio_usb(alertaTemperatura 1)

# Add the standard include files to the build
target_include_directories(alertaTemperatura PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/include/oled
  ${PICO_SDK_PATH}
  ${PICO_SDK_PATH}/src/common/pico_cyw43_arch/include
)

# Add any user requested libraries
target_link_libraries(alertaTemperatura 
        hardware_i2c
        hardware_pio
        hardware_dma
        hardware_spi
        hardware_i2c
        hardware_adc
        hardware_pwm
        hardware_timer
        hardware_clocks
        hardware_uart
        hardware_watchdog
        pico_stdlib
        pico_stdio
        )

pico_add_extra_outputs(alertaTemperatura)


