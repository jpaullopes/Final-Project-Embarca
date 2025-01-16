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

project(teste02 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(teste02 teste02.c )

pico_set_program_name(teste02 "teste02")
pico_set_program_version(teste02 "0.1")

# Generate PIO header
pico_generate_pio_header(teste02 ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(teste02 0)
pico_enable_stdio_usb(teste02 1)

# Add the standard library to the build
target_link_libraries(teste02
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(teste02 PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(teste02 
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
)

pico_add_extra_outputs(teste02)

# Add joystick project
project(joystick C CXX ASM)

# Add executable. Default name is the project name, version 0.1

add_executable(joystick joystick.c )

pico_set_program_name(joystick "joystick")
pico_set_program_version(joystick "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(joystick 0)
pico_enable_stdio_usb(joystick 1)

# Add the standard library to the build
target_link_libraries(joystick
        hardware_adc
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(joystick PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(joystick)
