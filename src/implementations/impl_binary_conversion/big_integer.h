#ifndef BIG_INTEGER_H
#define BIG_INTEGER_H

#include <smmintrin.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct big_integer {
    bool sign;
    size_t length;
    uint8_t *mem;
} big_integer;

/* Initialization of big_integers */
big_integer *create_big_integer(size_t bytes, bool sign);

big_integer *create_big_integer_of_bytes(size_t length, uint8_t bytes[length], bool sign);

big_integer *clone_big_integer(big_integer *og_big_integer);

void copy_big_integer_value_into_another(big_integer *source, big_integer *destination);

big_integer *clone_big_integer_add_size(big_integer *og_big_integer, size_t add_size);

/* size calculations */
size_t get_big_integer_min_size_exponentiation(int16_t base, int exponent);

size_t get_big_integer_min_size(int16_t base, size_t length);

/* getters and setters */
void set_byte_value_of_big_integer(big_integer *big_int, size_t index, uint8_t value);

void set_bit_value_of_big_integer(big_integer *big_int, size_t bit_index, bool value);

uint8_t get_byte_value_of_big_integer(big_integer *big_int, size_t index);

uint64_t get_7_bytes__of_big_integer(big_integer *big_int, size_t index);

void set_7_bytes__of_big_integer(big_integer *big_int, size_t index, uint64_t value);

__m128i get_15_bytes__of_big_integer(big_integer *big_int, size_t index);

void set_15_bytes__of_big_integer(big_integer *big_int, size_t index, __m128i value);

bool get_most_significant_bit(big_integer *value);

void set_zero(big_integer *value);

/* deletion */
void delete_big_integer(big_integer *value);

/* checks */
bool big_integer_is_zero(big_integer *value, bool simd);

bool big_integer_is_zero_simd(big_integer *value);

bool big_integer_is_equal(big_integer *a, big_integer *b);

/* negation */
void negate_big_integer(big_integer *value);

/* debug/IO */
void print_big_integer(big_integer *val);

void print_big_integer_hex(big_integer *val);

#endif
