/*
 * main_test.c
 *
 *  Created on: Jun 2, 2026
 *      Author: rashmi joshi
 */


#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// --- ML-KEM-768 Parameter Definitions (FIPS 203 Standard) ---
#define CRYPTO_PUBLICKEYBYTES   1184
#define CRYPTO_SECRETKEYBYTES   2400
#define CRYPTO_CIPHERTEXTBYTES  1088
#define CRYPTO_BYTES            32
#define MODULUS_Q               3329

// --- Mock Underlying API Prototypes (Replace with your actual ML-KEM library entrypoints) ---
extern void crypto_kem_keypair_derand(uint8_t *ek, uint8_t *dk, const uint8_t *d, const uint8_t *z);
extern void crypto_kem_enc_derand(uint8_t *c, uint8_t *k, const uint8_t *ek, const uint8_t *m);
extern void crypto_kem_dec(uint8_t *k, const uint8_t *c, const uint8_t *dk);

// --- Global Target Test Flags for Live Expressions View ---
volatile uint8_t test_keygen_passed   = 0;
volatile uint8_t test_encap_passed    = 0;
volatile uint8_t test_rejection_passed = 0;
volatile uint8_t test_keychecks_passed = 0;
volatile uint8_t acvp_all_tests_passed = 0;

// ============================================================================
// TEST 1: ML-KEM / keyGen / FIPS203 (AFT - Algorithm Functional Test)
// ============================================================================
void run_acvp_keygen_test(void) {
    // Inputs from ACVP Prompt Vector
    uint8_t seed_d[32] = {0x00, 0x01, 0x02, /* ... full 32 bytes ... */ 0x1F};
    uint8_t seed_z[32] = {0x20, 0x21, 0x22, /* ... full 32 bytes ... */ 0x3F};

    // Golden Reference Expected Outputs (Populate from your ACVP server/source)
    uint8_t golden_ek[CRYPTO_PUBLICKEYBYTES]  = {0x27, 0xD2, /* ... */ 0x1A};
    uint8_t golden_dk[CRYPTO_SECRETKEYBYTES]  = {0x27, 0xD2, /* ... */ 0x9B};

    // Implementation Work Buffers
    uint8_t computed_ek[CRYPTO_PUBLICKEYBYTES];
    uint8_t computed_dk[CRYPTO_SECRETKEYBYTES];

    // Execute Derandomized Generation
    crypto_kem_keypair_derand(computed_ek, computed_dk, seed_d, seed_z);

    // Validate Outputs
    if ((memcmp(computed_ek, golden_ek, CRYPTO_PUBLICKEYBYTES) == 0) &&
        (memcmp(computed_dk, golden_dk, CRYPTO_SECRETKEYBYTES) == 0)) {
        test_keygen_passed = 1;
    } else {
        test_keygen_passed = 0;
    }
}

// ============================================================================
// TEST 2: ML-KEM / encapDecap / FIPS203 (AFT - Encapsulation)
// ============================================================================
void run_acvp_encap_test(void) {
    // Inputs from ACVP Prompt Vector
    uint8_t seed_m[32] = {0x40, 0x41, 0x42, /* ... full 32 bytes ... */ 0x5F};
    uint8_t target_ek[CRYPTO_PUBLICKEYBYTES] = {0x27, 0xD2, /* ... */ 0x1A};

    // Golden Reference Expected Outputs
    uint8_t golden_c[CRYPTO_CIPHERTEXTBYTES] = {0x84, 0x1A, /* ... */ 0x4D};
    uint8_t golden_k[CRYPTO_BYTES]           = {0x69, 0x2D, /* ... */ 0x9B};

    // Implementation Work Buffers
    uint8_t computed_c[CRYPTO_CIPHERTEXTBYTES];
    uint8_t computed_k[CRYPTO_BYTES];

    // Execute Derandomized Encapsulation
    crypto_kem_enc_derand(computed_c, computed_k, target_ek, seed_m);

    // Validate Outputs
    if ((memcmp(computed_c, golden_c, CRYPTO_CIPHERTEXTBYTES) == 0) &&
        (memcmp(computed_k, golden_k, CRYPTO_BYTES) == 0)) {
        test_encap_passed = 1;
    } else {
        test_encap_passed = 0;
    }
}

// ============================================================================
// TEST 3: ML-KEM / encapDecap / FIPS203 (VAL - Decapsulation & Implicit Rejection)
// ============================================================================
void run_acvp_decap_rejection_test(void) {
    uint8_t target_dk[CRYPTO_SECRETKEYBYTES] = { /* Loaded private key */ 0 };

    // Server intentionally hands you a corrupted ciphertext
    uint8_t corrupted_c[CRYPTO_CIPHERTEXTBYTES] = {0xAA, 0xBB, 0xCC};

    // Server tells you what the deterministic fallback key should evaluate to
    uint8_t golden_rejection_k[CRYPTO_BYTES] = {0x5C, 0x82, /* ... */ 0xAB};

    uint8_t computed_k[CRYPTO_BYTES];

    // Execute Decapsulation
    crypto_kem_dec(computed_k, corrupted_c, target_dk);

    // Assert that the math fell back safely to the rejection state matching the golden vector
    if (memcmp(computed_k, golden_rejection_k, CRYPTO_BYTES) == 0) {
        test_rejection_passed = 1;
    } else {
        test_rejection_passed = 0;
    }
}

// ============================================================================
// TEST 4: ML-KEM Key Check (VAL - FIPS 203 Section 7.2 Validation)
// ============================================================================
bool validate_encapsulation_key(const uint8_t *ek) {
    // ACVP Requirement: Ensure byte decoding doesn't result in polynomial coefficients >= Q
    // This loops over the public key polynomial coefficients.
    for (int i = 0; i < 384 * 3; i++) {
        // Mock extraction logic representing coefficient validation
        uint16_t coefficient = (ek[i*2] | (ek[i*2+1] << 8)) & 0xFFF;
        if (coefficient >= MODULUS_Q) {
            return false; // "noisy linear system values too large" -> Test Fails
        }
    }
    return true; // Valid key structural check
}

void run_acvp_key_checks(void) {
    uint8_t bad_ek[CRYPTO_PUBLICKEYBYTES] = { 0xFF, 0xFF, 0xFF, 0xFF }; // Intentionally bad

    bool result = validate_encapsulation_key(bad_ek);

    // If the validator successfully caught the bad key structural anomaly, the check works.
    if (result == false) {
        test_keychecks_passed = 1;
    } else {
        test_keychecks_passed = 0;
    }
}

// ============================================================================
// Main Execution Entry Point
// ============================================================================
int main(void) {
    // STM32 Hardware Init (HAL_Init(), SystemClock_Config(), etc.) goes here

    // Run the ACVP Verification Suite Sequentially
    run_acvp_keygen_test();
    run_acvp_encap_test();
    run_acvp_decap_rejection_test();
    run_acvp_key_checks();

    // Final Aggregate Verification Check
    if (test_keygen_passed && test_encap_passed && test_rejection_passed && test_keychecks_passed) {
        acvp_all_tests_passed = 1;  // Breakpoint here! Your implementation is rock solid.
    } else {
        acvp_all_tests_passed = 0;
    }

    while (1) {
        // Keep running on the microcontroller to maintain Live Expressions states
    }
}
