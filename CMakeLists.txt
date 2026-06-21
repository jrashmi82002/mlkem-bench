cmake_minimum_required(VERSION 3.16)
project(mlkem_stm32 C)

set(CMAKE_C_STANDARD 11)

# Architecture compiler flags for Cortex-M4
add_compile_options(
    -mcpu=cortex-m4
    -mthumb
    -O0
    -Wall
)

# Linker scripts and flags to generate the bare-metal .elf executable
add_link_options(
    -T "${CMAKE_SOURCE_DIR}/STM32F411RETX_FLASH.ld"
    -mcpu=cortex-m4
    -mthumb
    --specs=nosys.specs
    -Wl,-Map=${PROJECT_NAME}.map,--gc-sections
)

add_executable(mlkem_stm32 src/main.c)