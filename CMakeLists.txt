cmake_minimum_required(VERSION 3.16)
project(hello_qemu C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# Bare-metal compiler target flags for Cortex-M4
add_compile_options(
    -mcpu=cortex-m4
    -mthumb
    -O0
    -Wall
)

# Custom entry point setup to compile directly without demanding an assembly startup file
add_link_options(
    -mcpu=cortex-m4
    -mthumb
    --specs=nosys.specs
    -Wl,-e,main
)

# Points explicitly to your folder structure: Src/main.c
add_executable(mlkem_stm32 Src/main.c)