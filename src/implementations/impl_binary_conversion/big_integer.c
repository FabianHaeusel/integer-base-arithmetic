#include "big_integer.h"

#include <smmintrin.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../util.h"
#include "arithmetic_helper.h"
#include "logger.h"

/*
 *
 * This file contains methods for the initialization and utility of big_integers.
 * The file big_integer_arithmetic.c contains big_integer arithmetic and comparisons.
 *
 */

// Definition of big_integer type in header file.
/*

    Definition of an arbitrary precision (big) integer.
    bool "sign" is the sign bit of the number. When set to true, the number is negative and vice
    versa. "length" specifies how many bytes the big_integer stores. "mem" is the pointer to the
    memory location where the bytes are stored. mem[0] stores the least significant byte.

    typedef struct big_integer {
        bool sign;
        size_t length;
        void* mem;
    } big_integer;

 */

/*
 * =====================================================================
 * Initialization (Initialization) of big integers
 * =====================================================================
 */

/**
 * Creates a zero-initialized big integer based on the number of bytes and the sign bit specified.
 * @param bytes Number of bytes the byte array of the integer should hold.
 * @param sign The sign of the big_integer: False = the integer positive, True = the integer is
 * negative.
 * @return The pointer to the created big_integer. Make sure to free this memory.
 */
big_integer *create_big_integer(size_t bytes, bool sign) {

    // Initialize big_integer struct (initialized memory).
    big_integer *bigint = (big_integer *) calloc(1, sizeof(big_integer));

    // Check if big_integer was initialized correctly
    check_alloc(bigint, sizeof(big_integer), "big_integer");

    // Allocate zero-initialized memory for the byte array
    bigint->mem = calloc(bytes, 1);

    // Check for NULL-Pointer in memory
    check_alloc(bigint, bytes, "big_integer->mem");

    // Assign sign and length of the big_integer
    bigint->sign = sign;
    bigint->length = bytes;

    return bigint;
}

/**
 * Creates a big_integer of the given number of bytes, byte values and sign of the int.
 * @param length The length of the big_integer.
 * @param bytes The byte values given as a byte array.
 * @param sign The sign indicating whether the big_integer is negative or positive.
 * @return The pointer to the created big_integer. Make sure to free this memory.
 */
big_integer *create_big_integer_of_bytes(size_t length, uint8_t bytes[length], bool sign) {
    big_integer *val = create_big_integer(length, sign);
    for (size_t i = 0; i < length; i++) {
        set_byte_value_of_big_integer(val, i, bytes[i]);
    }
    return val;
}

/**
 * Returns the minimum size of bytes a big_integer should have which can hold the result of the
 * exponentiation of the given base and exponent.
 * @param base The base of the numeral system as 16-bit signed integer: values in range [-128; 128].
 * @param exponent The exponent.
 */
size_t get_big_integer_min_size_exponentiation(int16_t base, int exponent) {
    // value base^exponent needs ceil(log_2(abs(base^exponent))) bits which can be computed by
    // exponent * ceil(log_2(abs(base))) using logarithmic algebraic laws.

    int bytes_needed = ((binary_logarithm_8bit_abs_ceil(base) * exponent) / 8) + 1;
    return bytes_needed;
}

/**
 * Returns the minimum size of bytes a big_integer should have which can hold any number with the
 * given base and the given amount of digits (number of digits when the value is represented in that
 * base).
 * @param base The base/radix of the system, base is element ( [-128;128] \ {-1;0;1} ).
 * @param length Length of digits of the number in the given base.
 */
size_t get_big_integer_min_size(int16_t base, size_t length) {
    // Create an array (memory location) of bytes that can hold the whole converted number with base
    // <base> and string input length <length>. The most significant char of a string of base b'
    // with b = abs(b') with length n has weight b^(n-1). The maximum value the whole string can
    // reach is (b^n)-1. Therefore, computing the binary logarithm on b^n (-1 can be left out) and
    // ceiling the result, dividing it by 8 to get the amount of bytes and add 1 will because
    // integer division rounds to floor, will guarantee that the biginteger is large enough to hold
    // any value with the specified base and length.

    // => We need a maximum of 'ceil(log_2(pow(abs(base), length))' digits to represent a number in
    // any base in binary. This is exactly what function create_big_integer_for_exponentiation
    // returns.
    return get_big_integer_min_size_exponentiation(base, length);
}

/*
 * =====================================================================
 * Big integer util
 * =====================================================================
 */

/**
 * Sets the byte value of the specified big_integer at the specified index to the specified value.
 * @param big_int The big_integer to change.
 * @param index The byte-index of which byte from the big_integer to change, where 0 refers to the
 * least-significant byte.
 * @param value The value which the byte gets changed to.
 */
void set_byte_value_of_big_integer(big_integer *big_int, size_t index, uint8_t value) {
    big_int->mem[index] = value;
}

/**
 * Set the bit value of the specified big_integer at the specified bit_index to the specified value
 * (either 1 or 0).
 * @param big_int The big_integer to change.
 * @param bit_index The bit_index, where 0 refers to the least-significant bit.
 * @param value The value which the bit gets changed to.
 */
void set_bit_value_of_big_integer(big_integer *big_int, size_t bit_index, bool value) {
    uint8_t byte = get_byte_value_of_big_integer(big_int, bit_index / 8);

    int i = ((int) bit_index % 8);
    if (value) {
        // set bit
        byte |= 0x01 << i;
    } else {
        // reset bit
        byte &= ~(0x01 << i);
    }

    set_byte_value_of_big_integer(big_int, bit_index / 8, byte);
}

/**
 * Returns the byte value of the specified byte-index in the specified big_integer.
 * @param big_int The big_integer to get the value from.
 * @param index The byte index, where 0 refers to the least significant byte.
 */
uint8_t get_byte_value_of_big_integer(big_integer *big_int, size_t index) {
    return big_int->mem[index];
}

uint64_t get_7_bytes__of_big_integer(big_integer *big_int, size_t index) {
    if (index + 6 >= big_int->length) {
        abort_err("Big_integer getting 7 bytes out of bounds. (Index: %zu, Length: %zu)\n", index,
                  big_int->length);
    }
    uint64_t val = 0;
    memcpy(&val, big_int->mem + index, 7);
    return val;
}

void set_7_bytes__of_big_integer(big_integer *big_int, size_t index, uint64_t value) {
    if (index + 6 >= big_int->length) {
        abort_err("Big_integer setting 7 bytes out of bounds. (Index: %zu, Length: %zu)\n", index,
                  big_int->length);
    }
    memcpy(big_int->mem + index, &value, 7);
}

__m128i get_15_bytes__of_big_integer(big_integer *big_int, size_t index) {
    if (index + 14 >= big_int->length) {
        abort_err("Big_integer getting 14 bytes out of bounds. (Index: %zu, Length: %zu)\n", index,
                  big_int->length);
    }

    __m128i val = _mm_setzero_si128();
    memcpy(&val, big_int->mem + index, 15);
    return val;
}

void set_15_bytes__of_big_integer(big_integer *big_int, size_t index, __m128i value) {
    if (index + 14 >= big_int->length) {
        abort_err("Big_integer setting 15 bytes out of bounds. (Index: %zu, Length: %zu)\n", index,
                  big_int->length);
    }
    memcpy(big_int->mem + index, &value, 15);
}

/**
 * Returns the most significant bit of the big_integer, which is the most significant bit of the
 * most significant byte of the big_integer.
 * @param value
 * @return
 */
bool get_most_significant_bit(big_integer *value) {
    return get_byte_value_of_big_integer(value, value->length - 1) & 0x80;
}

/**
 * Creates a new big_integer which has exactly the same value as the given one.
 * @param og_big_integer The big_integer to copy the values from.
 * @return The pointer to the created big_integer. Make sure to free this memory.
 */
big_integer *clone_big_integer(big_integer *og_big_integer) {
    big_integer *new_big_integer = create_big_integer(og_big_integer->length, og_big_integer->sign);
    // Copy the byte values of the og big integer to the newly created one.
    memcpy(new_big_integer->mem, og_big_integer->mem, og_big_integer->length);
    return new_big_integer;
}

/**
 * Sets the value of the big_integer to (positive) zero.
 * @param value
 */
void set_zero(big_integer *value) {
    for (size_t i = 0; i < value->length; i++) {
        set_byte_value_of_big_integer(value, i, 0);
    }
    value->sign = false;
}

/**
 * Copies the value of the source big_integer to the destination big_integer.
 * Warning: This function copies only as many values as the destination big_integer is big enough
 * for! Make sure the big_int is big enough.
 * @param source The big_integer to get the value from.
 * @param destination The big_integer to write the value into.
 */
void copy_big_integer_value_into_another(big_integer *source, big_integer *destination) {
    set_zero(destination);
    destination->sign = source->sign;

    memcpy(destination->mem, source->mem, min(source->length, destination->length));
}

/**
 * Copies the big_integer into a new one, but the one has add_size bytes more in memory than the
 * original one.
 * @param og_big_integer The big_integer to clone the values from.
 * @param add_size The number of bytes that are added on top of the length of the original big_integer to create the
 * new one.
 * @return
 */
big_integer *clone_big_integer_add_size(big_integer *og_big_integer, size_t add_size) {
    big_integer *new_big_integer =
            create_big_integer(og_big_integer->length + add_size, og_big_integer->sign);
    // Copy the byte values of the og big integer to the newly created one.
    memcpy(new_big_integer->mem, og_big_integer->mem, og_big_integer->length);

    return new_big_integer;
}

/**
 * Util/Debugging-function: Prints the big_integer value in binary representation.
 */
void print_big_integer(big_integer *val) {
    if (val->mem == NULL) {
        error("The big_integer you wanted to print is already deleted!");
        return;
    }

    printf(val->sign ? "- " : "+ ");
    for (int i = val->length - 1; i >= 0; i--) {
        uint8_t byte = ((uint8_t *) val->mem)[i];

        for (int j = 7; j >= 0; j--) {
            printf("%u", (byte >> j) & 0x1);
        }

        printf(" ");
    }

    printf("(length: %ld bytes) ", val->length);

    // Does not show values larger than the range of unsigned long integers.
    if (val->length <= 8) {
        unsigned long value = *((long *) val->mem);
        char prefix = val->sign ? '-' : '+';
        printf("(dec long: %c%lu)\n", prefix, value);
    } else
        printf("\n");
}

/**
 * Util/Debugging-function: Prints the big_integer value in hexadecimal representation.
 */
void print_big_integer_hex(big_integer *val) {
    if (val->mem == NULL) {
        error("The big_integer you wanted to print is already deleted!\n");
        return;
    }

    printf(val->sign ? "- " : "+ ");
    printf("0x ");
    for (int i = val->length - 1; i >= 0; i--) {
        uint8_t byte = (val->mem)[i];

        if (byte < 16) {
            printf("0");
        }

        printf("%X", byte);

        if (i % 8 == 0) {
            printf(" ");
        }
    }

    printf(" (length: %ld bytes) ", val->length);

    // Does not show values larger than the range of unsigned long integers.
    if (val->length <= 8) {
        unsigned long value = *((long *) val->mem);
        char prefix = val->sign ? '-' : '+';
        printf("(dec long: %c%lu)\n", prefix, value);
    } else
        printf("\n");
}

/**
 * Deletes the whole given big_integer.
 */
void delete_big_integer(big_integer *value) {
    if (value->mem == NULL) {
        abort_err("delete_big_integer() was called twice on a big_integer!");
    }

    free(value->mem);

    // set NULL to check if big_integer was already free'd (error handling)
    value->mem = NULL;

    free(value);
}

/**
 * Util function that returns true if the value of the given big_integer is (+/-) zero.
 * Complexity: Θ(n)
 */
bool big_integer_is_zero(big_integer *value, bool simd) {
    if (simd) return big_integer_is_zero_simd(value);

    for (size_t i = 0; i < value->length; i++) {
        uint8_t byte = get_byte_value_of_big_integer(value, i);
        if (byte != 0) return false;
    }
    return true;
}

bool big_integer_is_zero_simd(big_integer *value) {
    int i = 0;
    int length = (int) value->length;

    // 15 bytes at a time (SIMD 120 bit)
    for (; (i + 14) < length; i += 15) {
        __m128i vector = get_15_bytes__of_big_integer(value, i);
        int zero = _mm_testz_si128(vector, vector);
        // integer is 1 when all zero
        if (!zero) return false;
    }

    // 7 bytes at a time (SIMD 56 bit)
    for (; (i + 6) < length; i += 7) {
        uint64_t byte7 = get_7_bytes__of_big_integer(value, i);
        if (byte7 != 0) return false;
    }

    // 1 byte at a time (SISD)
    for (; i < length; i++) {
        uint8_t byte = get_byte_value_of_big_integer(value, i);
        if (byte != 0) return false;
    }
    return true;
}

/**
 * Checks if two big_integers are equal. +0 and -0 are not equal.
 */
bool big_integer_is_equal(big_integer *a, big_integer *b) {
    if (a->sign != b->sign) {
        return false;
    }
    if (big_integer_is_zero_simd(a) && big_integer_is_zero_simd(b)) {
        return true;
    }
    for (size_t i = 0; i < a->length || i < b->length; i++) {
        if (i < a->length && i < b->length) {
            if (get_byte_value_of_big_integer(a, i) != get_byte_value_of_big_integer(b, i)) {
                return false;
            }
        } else {
            if (i < a->length && get_byte_value_of_big_integer(a, i) != 0) return false;
            if (i < b->length && get_byte_value_of_big_integer(b, i) != 0) return false;
        }
    }
    return true;
}

/**
 * Negates the big_integer (inverts the sign bit).
 * @param value
 */
void negate_big_integer(big_integer *value) { value->sign = !value->sign; }
