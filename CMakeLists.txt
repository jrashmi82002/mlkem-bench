cmake_minimum_required(VERSION 3.16)
project(MLKEM_STM32_Project C ASM)

set(CMAKE_C_STANDARD 99)

# set TOOLCHAIN_ARM to TRUE if an ARM cross-compiler is detected
if(CMAKE_C_COMPILER_ID MATCHES "GNU" AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(TOOLCHAIN_ARM TRUE)
endif()
# Also allow it to be passed manually from the command line
if(CMAKE_TOOLCHAIN_FILE)
    set(TOOLCHAIN_ARM TRUE)
endif()

# 1. Grab all source files in Src
file(GLOB_RECURSE ALL_SOURCES 
    "Src/*.c"
    "Src/*.s"
    "Src/*.S"
)

# 2. Filter out main_test.c to fix the "multiple definition of main" error
list(REMOVE_ITEM ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Src/main_test.c")
list(REMOVE_ITEM ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Src/sha2.c")

# STM32 Hardware Target Build
if(TOOLCHAIN_ARM)
    add_executable(mlkem_stm32 
        Startup/startup_stm32f411retx.s
        ${ALL_SOURCES}
    )
    set_target_properties(mlkem_stm32 PROPERTIES SUFFIX ".elf")
    
    target_include_directories(mlkem_stm32 PRIVATE Inc)
    
    # STM32F411 Cortex-M4 Architecture Flags
    target_compile_options(mlkem_stm32 PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -O2
        -Wall
    )
    
    # Linker Flag Mapping
    target_link_options(mlkem_stm32 PRIVATE
        -T "${CMAKE_CURRENT_SOURCE_DIR}/STM32F411RETX_FLASH.ld"
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        --specs=nano.specs
        --specs=nosys.specs
    )
    
    # Post-Build Command
    # 🌟 FIX: Force outputs to drop into the current binary directory explicitly
    add_custom_command(TARGET mlkem_stm32 POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:mlkem_stm32> ${CMAKE_CURRENT_BINARY_DIR}/mlkem_stm32.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:mlkem_stm32> ${CMAKE_CURRENT_BINARY_DIR}/mlkem_stm32.bin
        COMMENT "Generated flashable binaries: mlkem_stm32.bin and mlkem_stm32.hex"
    )
endif()