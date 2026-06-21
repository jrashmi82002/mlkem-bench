cmake_minimum_required(VERSION 3.16)
project(hello_qemu C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

add_compile_options(-mcpu=cortex-m4 -mthumb -O0 -Wall)

# Tell the linker to put our vector table at the beginning of the file
add_link_options(
    -mcpu=cortex-m4 
    -mthumb 
    --specs=nano.specs       
    --specs=nosys.specs 
    -Wl,--section-start=.isr_vector=0x00000000
)

add_executable(mlkem_stm32 Src/main.c)