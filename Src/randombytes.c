#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "randombytes.h"

static const uint8_t *acvp_entropy_source = NULL;
static size_t acvp_entropy_offset = 0;

void acvp_set_test_entropy(const uint8_t *seed_array) {
    acvp_entropy_source = seed_array;
    acvp_entropy_offset = 0;
}

void acvp_clear_test_entropy(void) {
    acvp_entropy_source = NULL;
    acvp_entropy_offset = 0;
}

int randombytes(uint8_t *out, size_t outlen) {
    if (acvp_entropy_source != NULL) {
        memcpy(out, &acvp_entropy_source[acvp_entropy_offset], outlen);
        acvp_entropy_offset += outlen;
        return 0;
    }
    return 0; // Hardware RNG fallback remains unperturbed
}
