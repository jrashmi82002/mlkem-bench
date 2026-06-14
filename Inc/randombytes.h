//#ifndef RANDOMBYTES_H_
//#define RANDOMBYTES_H_
//
//#include <stddef.h>
//#include <stdint.h>
//
//int randombytes(uint8_t *out, size_t outlen);
//
//#endif /* RANDOMBYTES_H_ */


//with NIST Vectors
#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

// Standard cryptographic API signature
int randombytes(uint8_t *out, size_t outlen);

// Mode Changers
void use_official_nist_kat_mode(void);
void set_custom_text_seed(const char* text_input);

// Mode Status Flag (0 = Custom Text, 1 = Official NIST Vector)
extern volatile int nist_mode_active;

// Boundary verification arrays exported to main.c
extern const uint8_t nist_gold_pk_start[2];
extern const uint8_t nist_gold_pk_end[2];
extern const uint8_t nist_gold_ct_start[2];
extern const uint8_t nist_gold_ct_end[2];
extern const uint8_t nist_gold_ss_start[2];
extern const uint8_t nist_gold_ss_end[2];

#endif /* RANDOMBYTES_H */
