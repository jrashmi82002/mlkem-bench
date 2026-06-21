cmake_minimum_required(VERSION 3.16)
project(MLKEM_STM32_Project C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

file(GLOB_RECURSE ALL_SOURCES 
    "Src/*.c"
    "Src/*.s"
    "Src/*.S"
)

list(REMOVE_ITEM ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Src/main_test.c")
list(REMOVE_ITEM ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Src/sha2.c")

if(TOOLCHAIN_ARM)
    add_executable(mlkem_stm32 
        Startup/startup_stm32f411retx.s
        ${ALL_SOURCES}
    )
    
    target_include_directories(mlkem_stm32 PRIVATE Inc)
    
    # Added -DEMULATION globally so conditional execution routes cleanly
    target_compile_options(mlkem_stm32 PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -O2
        -Wall
        -DEMULATION
    )
    
    # Removed --specs=nosys.specs to guarantee our custom _write symbol overlays clean
    target_link_options(mlkem_stm32 PRIVATE
        -T "${CMAKE_CURRENT_SOURCE_DIR}/STM32F411RETX_FLASH.ld"
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        --specs=nano.specs
    )
    
    add_custom_command(TARGET mlkem_stm32 POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:mlkem_stm32> mlkem_stm32.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:mlkem_stm32> mlkem_stm32.bin
        COMMENT "Generated flashable binaries: mlkem_stm32.bin and mlkem_stm32.hex"
    )
endif()