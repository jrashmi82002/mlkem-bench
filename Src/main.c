#include <stdint.h>

// Bare-metal Memory Mapping for USART2 & RCC
#define RCC_BASE          0x40023800
#define GPIOA_BASE        0x40020000

#define RCC_AHB1ENR       (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR       (*(volatile uint32_t*)(RCC_BASE + 0x40))
#define GPIOA_MODER       (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL (*(volatile uint32_t *)(GPIOA_BASE + 0x20))

typedef struct
{
    volatile uint32_t SR;  // Status register
    volatile uint32_t DR;  // Data register
    volatile uint32_t BRR; // Baud rate register
    volatile uint32_t CR1; // Control register 1
} USART_TypeDef;

#define USART2 ((USART_TypeDef *)0x40004400)

void ConfigureUSART2(void)
{
    RCC_AHB1ENR |= (1 << 0);            // 1. Enable GPIOA Clock
    RCC_APB1ENR |= (1 << 17);           // 2. Enable USART2 Clock
    GPIOA_MODER &= ~(3 << 4);           // 3. Reset Pin A2
    GPIOA_MODER |= (2 << 4);            // 4. Set Pin A2 to Alternate Function
    GPIOA_AFRL &= ~(0xF << 8);          // 5. Reset Pin A2 low nibble
    GPIOA_AFRL |= (7 << 8);             // 6. Bind Pin A2 to AF7 (USART2 TX)

    USART2->BRR = 0x0683;               // 7. Standard 115200 baud configuration
    USART2->CR1 = (1 << 13) | (1 << 3); // 8. Globally enable USART Block and Transmitter
}

void USART2_PrintString(const char *str)
{
    while (*str)
    {
        while (!(USART2->SR & (1 << 7)))
            ;                         // Wait until Transmit Data Register is empty
        USART2->DR = (*str++ & 0xFF); // Send byte to QEMU character backend
    }
}

int main(void)
{
    ConfigureUSART2();

    // Print repeatedly to ensure QEMU catches it before the timeout
    for (int i = 0; i < 10; i++)
    {
        USART2_PrintString("Hello from QEMU!\r\n");
    }

    // Explicitly halt the CPU so it doesn't run away or crash
    while (1)
    {
        __asm("nop");
    }
}