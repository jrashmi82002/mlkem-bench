cmake_minimum_required(VERSION 3.16)
project(hello_qemu C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

add_compile_options(
    -mcpu=cortex-m4 
    -mthumb 
    -O0 
    -Wall
)

add_link_options(
    -mcpu=cortex-m4 
    -mthumb 
    --specs=nosys.specs 
    -Wl,--section-start=.isr_vector=0x08000000
)

add_executable(mlkem_stm32 Src/main.c)