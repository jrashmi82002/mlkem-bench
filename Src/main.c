#include <stdio.h>
#include <stdint.h>

// Memory map for USART2 data register
#define USART2_DR (*(volatile uint32_t *)(0x40004400 + 0x04))

// Minimal stack definition
static uint32_t stack[1024];

void Reset_Handler(void);

// Clean vector table aligned by the linker script
__attribute__((section(".isr_vector"), used)) void (*const vectors[])(void) = {
    (void (*)(void))(&stack[1024]),
    Reset_Handler};

// Wiring printf directly into QEMU's simulated console channel
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        USART2_DR = (ptr[i] & 0xFF);
    }
    return len;
}

void Reset_Handler(void)
{
    main();
    while (1)
        ;
}

int main(void) {
    // No messy loops or register setups needed. QEMU handles printf immediately!
    for (int i = 0; i < 10; i++)
    {
        printf("Hello from QEMU! ML-KEM Benchmarking Environment Online.\r\n");
    }

    while (1) {
        __asm("nop");
    }
}