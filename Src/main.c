#include <stdint.h>

// --- Bare-metal Memory Mapping ---
#define RCC_AHB1ENR       (*(volatile uint32_t*)(0x40023800 + 0x30))
#define RCC_APB1ENR       (*(volatile uint32_t*)(0x40023800 + 0x40))
#define GPIOA_MODER       (*(volatile uint32_t*)(0x40020000 + 0x00))
#define GPIOA_AFRL        (*(volatile uint32_t*)(0x40020000 + 0x20))

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
} USART_TypeDef;

#define USART2 ((USART_TypeDef *)0x40004400)

// --- Forward Declarations ---
int main(void);
void Reset_Handler(void);

// --- The Vector Table Needed for QEMU ---
static uint32_t stack[1024]; 
void Reset_Handler(void) { 
    main(); 
    while(1); 
}

__attribute__((section(".isr_vector"), used))
void (*const vectors[])(void) = { (void (*)(void))(&stack[1024]), Reset_Handler };

// --- Simplified USART Functions for QEMU ---
void ConfigureUSART2(void) {
    RCC_AHB1ENR |= (1 << 0);            
    RCC_APB1ENR |= (1 << 17);           
    GPIOA_MODER &= ~(3 << 4);           
    GPIOA_MODER |= (2 << 4);            
    GPIOA_AFRL &= ~(0xF << 8);          
    GPIOA_AFRL |= (7 << 8);             
    USART2->BRR = 0x0683;               
    USART2->CR1 = (1 << 13) | (1 << 3); 
}

void USART2_PrintString(const char *str) {
    while (*str) {
        // 🌟 FIX: Removed the status register loop trap!
        // QEMU doesn't emulate hardware flags natively, so we write directly.
        USART2->DR = (*str++ & 0xFF);
    }
}

int main(void) {
    ConfigureUSART2();
    
    // Print several times to ensure it hits the log stream cleanly
    for (int i = 0; i < 10; i++) {
        USART2_PrintString("Hello from QEMU!\r\n");
    }

    // Explicitly halt the emulation core so it stops smoothly
    while (1) {
        __asm("nop");
    }
}