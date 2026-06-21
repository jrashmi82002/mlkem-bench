cmake_minimum_required(VERSION 3.16)
project(hello_qemu C)

set(CMAKE_C_STANDARD 11)

# Bare-metal compile flags
add_compile_options(
    -mcpu=cortex-m4
    -mthumb
    -O0
    -Wall
)

# FORCE A GENERIC ENTRY POINT: Bypasses missing Reset_Handler / startup scripts
add_link_options(
    -mcpu=cortex-m4
    -mthumb
    --specs=nosys.specs
    -Wl,-e,main
)

add_executable(mlkem_stm32 Src/main.c)