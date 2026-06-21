cmake_minimum_required(VERSION 3.16)
project(mlkem_bench C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

add_compile_options(
    -mcpu=cortex-m4 
    -mthumb 
    -O0 
    -Wall
)

# Use our clean linker script and include standard bare-metal specifications
add_link_options(
    -mcpu=cortex-m4 
    -mthumb 
    -T "${CMAKE_CURRENT_SOURCE_DIR}/stm32f4.ld"
    --specs=nosys.specs
)

add_executable(mlkem_stm32 Src/main.c)