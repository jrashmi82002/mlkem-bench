#include <stdint.h>
#include <sys/types.h>

#define RCC_AHB1ENR (*(volatile uint32_t *)(0x40023800 + 0x30))
#define RCC_APB1ENR (*(volatile uint32_t *)(0x40023800 + 0x40))
#define GPIOA_MODER (*(volatile uint32_t *)(0x40020000 + 0x00))
#define GPIOA_AFRL (*(volatile uint32_t *)(0x40020000 + 0x20))

typedef struct
{
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
} USART_TypeDef;

#define USART2 ((USART_TypeDef *)0x40004400)

void ConfigureUSART2(void)
{
    RCC_AHB1ENR |= (1 << 0);
    RCC_APB1ENR |= (1 << 17);
    GPIOA_MODER &= ~(3 << 4);
    GPIOA_MODER |= (2 << 4);
    GPIOA_AFRL &= ~(0xF << 8);
    GPIOA_AFRL |= (7 << 8);
    USART2->BRR = 0x0683;
    USART2->CR1 = (1 << 13) | (1 << 3);
}

void USART2_PrintString(const char *str)
{
    while (*str)
    {
        while (!(USART2->SR & (1 << 7)))
            ;
        USART2->DR = (*str++ & 0xFF);
    }
}

// Minimal bare-metal OS system stubs to make newlib link perfectly
int _close(int file) { return -1; }
int _fstat(int file, void *st) { return 0; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
caddr_t _sbrk(int incr)
{
    static char *heap_end;
    return (caddr_t)-1;
}
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        while (!(USART2->SR & (1 << 7)))
            ;
        USART2->DR = (ptr[i] & 0xFF);
    }
    return len;
}

int main(void)
{
    ConfigureUSART2();

    for (int i = 0; i < 10; i++)
    {
        USART2_PrintString("Hello from QEMU!\r\n");
    }

    while (1)
    {
        __asm("nop");
    }
}