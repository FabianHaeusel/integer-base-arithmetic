#ifndef BIG_INTEGER_ARITHMETIC_H
#define BIG_INTEGER_ARITHMETIC_H

#include "big_integer.h"

/* arithmetic */
void big_integer_addition(big_integer *value_a, big_integer *value_b, bool simd);

void big_integer_subtraction(big_integer *value_a, big_integer *value_b, bool simd);

void big_integer_increment(big_integer *value);

/* binary shift */
void big_integer_shl_bitwise_0_to_7(big_integer *value, uint8_t bit_count, bool simd);

void big_integer_shl_bitwise_0_to_7__sisd(big_integer *value, uint8_t bit_count);

void big_integer_shl_bitwise_0_to_7__simd56(big_integer *value, uint8_t bit_count);

void big_integer_shl_byte_wise(big_integer *value, size_t count);

/* multiplication */
void big_integer_multiply_uint8(big_integer *value, uint8_t mul, big_integer *result,
                                big_integer *temp, bool simd);

void big_integer_multiplication(big_integer *a, big_integer *b, big_integer *res, bool simd);

void big_integer_multiply_int_neg256_to_256(big_integer *value, int16_t mul, big_integer *result,
                                            big_integer *temp, bool simd);

/* division */
int16_t big_integer_division_int9_t(big_integer *value, int16_t divisor, big_integer *temp_value,
                                    big_integer *temp_value2, bool simd);

/* comparison */
bool positive_big_integer_is_greater_than(big_integer *a, big_integer *b, bool simd);

bool big_integer_greater_equal_int16(big_integer *a, int16_t b, bool simd);

#endif
