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

project(embarcaTechProject C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
# Confirma que apenas os arquivos atualizados serão compilados.
add_executable(embarcaTechProject embarcaTechProject.c  inc/ssd1306_i2c.c)

pico_set_program_name(embarcaTechProject "embarcaTechProject")
pico_set_program_version(embarcaTechProject "0.1")

# Generate PIO header
pico_generate_pio_header(embarcaTechProject ${CMAKE_CURRENT_LIST_DIR}/blink.pio)
pico_generate_pio_header(embarcaTechProject ${CMAKE_CURRENT_LIST_DIR}/ws2818b.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(embarcaTechProject 0)
pico_enable_stdio_usb(embarcaTechProject 1)

# Add the standard library to the build
target_link_libraries(embarcaTechProject
        pico_stdlib
        pico_cyw43_arch_lwip_poll
        hardware_i2c
        hardware_adc
        hardware_pwm)

# Add the standard include files to the build
target_include_directories(embarcaTechProject PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_SDK_PATH}
        ${PICO_SDK_PATH}/src/common/pico_cyw43_arch/include
)

pico_add_extra_outputs(embarcaTechProject)

