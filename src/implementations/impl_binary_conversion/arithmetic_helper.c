#include <stdint.h>
#include <stdlib.h>

/**
 * Computes the binary logarithm of a signed integer (16 bit value which should only hold values between -256 and 256.)
 * and rounds it to positive infinity if the result is positive or zero and to negative infinity if the result is
 * negative.
 */
int binary_logarithm_8bit_abs_ceil(int16_t value) {
    int powers[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    for (int i = 7; i >= 0; i--) {
        if (abs(value) > powers[i]) return i + 1;
    }
    return -1;
}

/**
 * Returns the maximum of two size_t values.
 */
size_t max(size_t a, size_t b) {
    if (a > b) return a;
    return b;
}

size_t min(size_t a, size_t b) {
    if (a < b) return a;
    return b;
}