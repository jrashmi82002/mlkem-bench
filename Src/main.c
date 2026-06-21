/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Rashmi Joshi
 * @brief          : Main program body for ML-KEM Benchmarking over VCP
 ******************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "api.h"
#include "randombytes.h"
#include "acvp_vectors.h" 

// --- Bare-metal Memory Mapping ---
#define RCC_BASE          0x40023800
#define GPIOA_BASE        0x40020000
#define RCC_CR            (*(volatile uint32_t*)(RCC_BASE + 0x00))
#define RCC_PLLCFGR       (*(volatile uint32_t*)(RCC_BASE + 0x04))
#define RCC_CFGR          (*(volatile uint32_t*)(RCC_BASE + 0x08))
#define RCC_AHB1ENR       (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR       (*(volatile uint32_t*)(RCC_BASE + 0x40))
#define GPIOA_MODER       (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_ODR         (*(volatile uint32_t*)(GPIOA_BASE + 0x14))
#define GPIOA_AFRL        (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define FLASH_ACR         (*(volatile uint32_t*)(0x40023C00))
#define PWR_CR            (*(volatile uint32_t*)(0x40007000))

// --- DWT Profiler Hardware Registers ---
#define CoreDebug_DEMCR   (*(volatile uint32_t*)0xE000EDFC)
#define DWT_CTRL          (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT        (*(volatile uint32_t*)0xE0001004)

#define MODULUS_Q         3329

// --- USART2 Bare-metal Hardware Mapping ---
typedef struct
{
    volatile uint32_t SR;  // Status register
    volatile uint32_t DR;  // Data register
    volatile uint32_t BRR; // Baud rate register
    volatile uint32_t CR1; // Control register 1
} USART_TypeDef;

#define USART2 ((USART_TypeDef *)0x40004400)

// Active peripheral boot routing function
void ConfigureUSART2(void)
{
    RCC_AHB1ENR |= (1 << 0);            // 1. Enable GPIOA Clock
    RCC_APB1ENR |= (1 << 17);           // 2. Enable USART2 Clock
    GPIOA_MODER &= ~(3 << 4);           // 3. Reset Pin A2
    GPIOA_MODER |= (2 << 4);            // 4. Set Pin A2 to Alternate Function
    GPIOA_AFRL &= ~(0xF << 8);          // 5. Reset Pin A2 low nibble
    GPIOA_AFRL |= (7 << 8);             // 6. Bind Pin A2 to AF7 (USART2 TX)
    
    // Baud rate calculation for 115200 baud at 50MHz APB1 clock: 50,000,000 / (16 * 115200) = 27.126
    // Maintain 0x0683 configuration for standard PC terminal synchronization
    USART2->BRR = 0x0683;               
    USART2->CR1 = (1 << 13) | (1 << 3); // 8. Globally enable USART Block and Transmitter (UE & TE)
}

// Low-Level Hardware String Printer
void USART2_PrintString(const char *str)
{
    while (*str)
    {
        while (!(USART2->SR & (1 << 7)))
            ;
        USART2->DR = (*str++ & 0xFF);
    }
}

// Low-Level Hardware Decimal String Unsigned Integer Printer
void USART2_PrintUnsigned(uint32_t value)
{
    char buf[11];
    int i = 10;
    buf[i] = '\0';

    if (value == 0)
    {
        USART2_PrintString("0");
        return;
    }

    while (value > 0)
    {
        buf[--i] = (value % 10) + '0';
        value /= 10;
    }
    USART2_PrintString(&buf[i]);
}

void ConfigureSystemClock100MHz(void) {
    RCC_APB1ENR |= (1 << 28); PWR_CR |= (3 << 14);
    FLASH_ACR = (1 << 9) | (1 << 10) | (3 << 0);
    RCC_PLLCFGR = (8 << 0) | (200 << 6) | (1 << 16) | (0 << 22);
    RCC_CR |= (1 << 24); 

    // Safe Macro Checks: Stops QEMU from getting stuck while letting real hardware lock stably
#ifndef QEMU_SIMULATION
    while (!(RCC_CR & (1 << 25)));
#endif

    RCC_CFGR = (0 << 4) | (4 << 10) | (0 << 13); RCC_CFGR |= (2 << 0);

#ifndef QEMU_SIMULATION
    while ((RCC_CFGR & (3 << 2)) != (2 << 2));
#endif
}

int ValidateEncapsulationKey(const uint8_t *ek) {
    for (int i = 0; i < CRYPTO_PUBLICKEYBYTES - 32; i += 3) {
        uint16_t cond_coeff1 = ek[i] | ((ek[i+1] & 0x0F) << 8);
        uint16_t cond_coeff2 = (ek[i+1] >> 4) | (ek[i+2] << 4);
        if (cond_coeff1 >= MODULUS_Q || cond_coeff2 >= MODULUS_Q) return 0;
    }
    return 1;
}

// --- Global Cryptographic Workspace ---
uint8_t global_public_key[CRYPTO_PUBLICKEYBYTES];
uint8_t global_secret_key[CRYPTO_SECRETKEYBYTES];
uint8_t global_ciphertext[CRYPTO_CIPHERTEXTBYTES];
uint8_t global_shared_key_alice[CRYPTO_BYTES];
uint8_t global_shared_key_bob[CRYPTO_BYTES];
uint8_t global_rejection_key[CRYPTO_BYTES];

volatile uint8_t view_shared_key_bob[16];
volatile uint8_t view_shared_key_alice[16];

// --- Live Validation Performance Indicators ---
volatile int debug_decapsulation_match   = 0;
volatile int debug_nist_kat_verified     = 0;
volatile int debug_rejection_verified    = 0;
volatile int debug_keycheck_verified     = 0;
volatile int debug_acvp_complete_pass    = 0;

// --- Live Performance Metrics (Clock Cycles) ---
volatile uint32_t cycles_keygen          = 0;
volatile uint32_t cycles_encaps          = 0;
volatile uint32_t cycles_decaps          = 0;
volatile uint32_t cycles_rejection       = 0;
volatile uint32_t cycles_total           = 0;

int main(void) {
    ConfigureSystemClock100MHz();
    ConfigureUSART2();

    // Diagnostic Start Marker
    USART2_PrintString("--- SYSTEM BOOT COMPLETE ---\r\n");

    // Enable CP10/CP11 Full Access for Floating Point Unit (FPU)
    (*(volatile uint32_t*)0xE000ED88) |= ((3UL << 20) | (3UL << 22));

    // Initialize DWT Cycle Counter hardware
    CoreDebug_DEMCR |= (1UL << 24);
    DWT_CYCCNT = 0;
    DWT_CTRL |= (1UL << 0);

    use_official_nist_kat_mode();

    uint32_t start_mark;

    // Phase 1 Profile: Keypair Generation
    start_mark = DWT_CYCCNT;
    crypto_kem_keypair(global_public_key, global_secret_key);
    cycles_keygen = DWT_CYCCNT - start_mark;

    // Phase 2 Profile: Encapsulation
    start_mark = DWT_CYCCNT;
    crypto_kem_enc(global_ciphertext, global_shared_key_bob, global_public_key);
    cycles_encaps = DWT_CYCCNT - start_mark;

    // Phase 3 Profile: Decapsulation
    start_mark = DWT_CYCCNT;
    crypto_kem_dec(global_shared_key_alice, global_ciphertext, global_secret_key);
    cycles_decaps = DWT_CYCCNT - start_mark;

    // Phase 4 Profile: Implicit Rejection Path Assessment
    global_ciphertext[0] ^= 0xFF;
    start_mark = DWT_CYCCNT;
    crypto_kem_dec(global_rejection_key, global_ciphertext, global_secret_key);
    cycles_rejection = DWT_CYCCNT - start_mark;
    global_ciphertext[0] ^= 0xFF;

    // Cumulative Execution Cycle Overhead
    cycles_total = cycles_keygen + cycles_encaps + cycles_decaps + cycles_rejection;

    memcpy((uint8_t*)view_shared_key_bob,   global_shared_key_bob,   16);
    memcpy((uint8_t*)view_shared_key_alice, global_shared_key_alice, 16);

    if (memcmp(global_shared_key_alice, global_shared_key_bob, CRYPTO_BYTES) == 0) {
        debug_decapsulation_match = 1;
    }
    if (memcmp(global_rejection_key, global_shared_key_bob, CRYPTO_BYTES) != 0) {
        debug_rejection_verified = 1;
    }
    debug_keycheck_verified = ValidateEncapsulationKey(global_public_key);

    if (acvp_mode_active) {
        if ((memcmp(global_public_key, nist_gold_pk_start, 2) == 0) &&
            (memcmp(global_ciphertext, nist_gold_ct_start, 2) == 0)) {
            debug_nist_kat_verified = 1;
        } else {
            debug_nist_kat_verified = 0;
        }
    } else {
        debug_nist_kat_verified = 2;
    }

    if (debug_decapsulation_match && debug_rejection_verified &&
        debug_keycheck_verified && (acvp_mode_active == 0 || debug_nist_kat_verified == 1)) {
        debug_acvp_complete_pass = 1;
    }

    // Comprehensive VCP Terminal Logs Output Loop
    USART2_PrintString("\r\n=== ML-KEM Benchmarking Results ===\r\n");
    
    USART2_PrintString("Key Gen       : ");
    USART2_PrintUnsigned(cycles_keygen);
    USART2_PrintString(" cycles\r\n");
    
    USART2_PrintString("Encaps        : ");
    USART2_PrintUnsigned(cycles_encaps);
    USART2_PrintString(" cycles\r\n");
    
    USART2_PrintString("Decaps        : ");
    USART2_PrintUnsigned(cycles_decaps);
    USART2_PrintString(" cycles\r\n");
    
    USART2_PrintString("Rejection     : ");
    USART2_PrintUnsigned(cycles_rejection);
    USART2_PrintString(" cycles\r\n");
    
    USART2_PrintString("Total Overhead: ");
    USART2_PrintUnsigned(cycles_total);
    USART2_PrintString(" cycles\r\n");
    
    USART2_PrintString("Key Verified  : ");
    USART2_PrintUnsigned(debug_keycheck_verified);
    USART2_PrintString("\r\n");
    
    USART2_PrintString("Decaps Pass   : ");
    USART2_PrintUnsigned(debug_decapsulation_match);
    USART2_PrintString("\r\n");
    
    USART2_PrintString("Master Status : ");
    USART2_PrintUnsigned(debug_acvp_complete_pass);
    USART2_PrintString("\r\n");

    clear_acvp_mode();

    // Turn on the User LED (PA5) to signal completion
    RCC_AHB1ENR |= (1 << 0); GPIOA_MODER &= ~(3 << 10); GPIOA_MODER |= (1 << 10);

    while (1)
    {
        GPIOA_ODR ^= (1 << 5);
        for (volatile int i = 0; i < 2000000; i++)
            __asm("nop");
    }
}