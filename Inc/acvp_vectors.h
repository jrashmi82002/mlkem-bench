/*
 * acvp_vectors.h
 *
 *  Created on: Jun 2, 2026
 *      Author: rashmi joshi
 */

#ifndef ACVP_VECTORS_H
#define ACVP_VECTORS_H

#include <stdint.h>

// Global mode tracker flag
extern volatile int acvp_mode_active;

// Golden reference signatures exposed for verification checks
extern const uint8_t nist_gold_pk_start[2];
extern const uint8_t nist_gold_ct_start[2];

// High-level initializer functions to swap modes cleanly
void use_official_nist_kat_mode(void);
void clear_acvp_mode(void);

#endif // ACVP_VECTORS_H
