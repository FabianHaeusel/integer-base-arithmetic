#ifndef IMPL_BINARY_CONVERSION_H
#define IMPL_BINARY_CONVERSION_H

#include <stdbool.h>
#include <stdint.h>

#include "big_integer.h"

/* core functions */
void arith_op_any_base__binary_conversion__sisd(int base, const char *alph, const char *z1,
                                                const char *z2, char op, char *result);

void arith_op_any_base__binary_conversion__simd(int base, const char *alph, const char *z1,
                                                const char *z2, char op, char *result);

void arith_op_any_base__binary_conversion(int base, const char *alph, const char *z1,
                                          const char *z2, char op, char *result, bool simd);

/* alphabet util */
uint8_t get_char_value(char c, uint8_t (*lookup)[]);

void create_alphabet_lookup(const char *alph, uint8_t (*lookup)[]);

/* conversion */
void convert_numbers_from_any_base_into_binary(int base, const char *alph, const char *z1,
                                               const char *z2, size_t z1_length, size_t z2_length,
                                               big_integer *z1_binary, big_integer *z2_binary,
                                               bool simd);

void convert_big_integer_to_any_base(big_integer *value, int16_t base, const char *alph,
                                     char *buffer, size_t buffer_length, bool simd);

#endif
