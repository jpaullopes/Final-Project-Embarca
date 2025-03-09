cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

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

# Define o Pico W como placa-alvo
set(PICO_BOARD pico_w)

# Importa o SDK do Pico (deve vir antes do projeto)
include(pico_sdk_import.cmake)

project(wifiConect C CXX ASM)

# Inicializa o SDK do Raspberry Pi Pico
pico_sdk_init()

# Adiciona a biblioteca WiFi
add_library(pico_wifi_lib STATIC
    pico_wifi_lib.c
)

# Define as dependências da biblioteca WiFi
target_link_libraries(pico_wifi_lib
    pico_stdlib
    pico_cyw43_arch_lwip_poll
)

# Inclui os diretórios necessários para a biblioteca
target_include_directories(pico_wifi_lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Adiciona o executável principal
add_executable(wifiConect wifiConect.c)

# Habilita USB para printf
pico_enable_stdio_usb(wifiConect 1)
pico_enable_stdio_uart(wifiConect 0)

# Adicionando diretório include para encontrar lwipopts.h
target_include_directories(wifiConect PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Vincula as bibliotecas necessárias, incluindo nossa biblioteca WiFi
target_link_libraries(wifiConect
    pico_stdlib
    pico_wifi_lib  # Nossa biblioteca WiFi agora é uma dependência
)

# Gera arquivos binários adicionais
pico_add_extra_outputs(wifiConect)