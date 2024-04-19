#include "impl_naive.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../../test.h"
#include "../../util.h"
#include "../common.h"

/**
 * @brief Add or subtract two unsigned numbers
 *
 * z1 has to be bigger than z2 for subtraction.
 * The NULL terminated result will be written to the given buffer.
 * If specified the result will be prefixed by a '-' character.
 *
 * @param add       True for addition, false for subtraction
 * @param negate    True if the result shall be prefixed by '-'
 * @param base      The base
 * @param alph      The alphabet
 * @param z1        The first number (augend/minuend)
 * @param z2        The second number (addend/subtrahend)
 * @param result    The result buffer
 */
static void add_sub_unsigned(bool add, bool negate, int base, const char *alph, const char *z1,
                             const char *z2, char *result) {
    // if base < 0 carry sign is inverted
    int carry_offset = base < 0 ? -1 : 1;

    unsigned int base_abs = abs(base);

    // populate lookup table (lut[digit] == index of digit in alphabet)
    unsigned char lut[UCHAR_MAX + 1];
    generate_lut(lut, base_abs, alph);

    size_t a_len = strlen(z1);
    size_t b_len = strlen(z2);

    const char *a_ms_digit = z1;
    const char *b_ms_digit = z2;

    // initialize current digit with pointer to last digit
    const char *current_digit_a = a_ms_digit + a_len - 1;
    const char *current_digit_b = b_ms_digit + b_len - 1;

    // write result backwards and reverse at the end
    char *current_digit_result = result;

    int carry = 0;

    // add with carry as long as there are digits left in a or b or carry != 0
    while (current_digit_a >= a_ms_digit || current_digit_b >= b_ms_digit || carry != 0) {
        char a;
        char b;

        // if a or b has no more digits use 0 respectively
        if (current_digit_a < a_ms_digit) {
            a = alph[0];
        } else {
            a = *current_digit_a;
        }

        if (current_digit_b < b_ms_digit) {
            b = alph[0];
        } else {
            b = *current_digit_b;
        }

        unsigned char a_idx = lut[(unsigned char) a];
        unsigned char b_idx = lut[(unsigned char) b];

        // calculate the sum/difference of the indices of a and b + carry
        long sum_idx = (add ? a_idx + b_idx : a_idx - b_idx) + carry;

        // handle carry by adding/subtracting abs(base) from the result and assign a new carry value
        if (sum_idx >= base_abs) {
            // carryAdd
            *current_digit_result = alph[sum_idx - base_abs];
            carry = carry_offset;
        } else if (sum_idx < 0) {
            // carrySub
            *current_digit_result = alph[sum_idx + base_abs];
            carry = -carry_offset;
        } else {
            // no carry
            *current_digit_result = alph[sum_idx];
            carry = 0;
        }

        // decrement a and b to the next-most significant digit
        current_digit_a--;
        current_digit_b--;
        // increment result pointer to the next-most significant digit
        current_digit_result++;
    }

    // strip leading zeros
    while (current_digit_result > (result + 1) && *(current_digit_result - 1) == alph[0]) {
        current_digit_result--;
    }

    // add '-' if result has to be negated
    if (negate && *(current_digit_result - 1) != alph[0]) {
        *current_digit_result = '-';
        current_digit_result++;
    }

    // terminate string with NULL byte
    *current_digit_result = '\0';

    // reverse string
    size_t len = strlen(result);
    for (size_t i = 0, j = len - 1; i < j; i++, j--) {
        char tmp = result[i];
        result[i] = result[j];
        result[j] = tmp;
    }
}

/**
 * @brief Compare two unsigned numbers with base > 0
 *
 * @param base      Base of the two numbers
 * @param alph      Alphabet of the numbers
 * @param z1        First number
 * @param z2        Second number
 * @return          -1 if first number is smaller, 1 if first number is greater than second and 0 if
 *                  z1 == z2
 */
static int cmp_unsigned_pos_base(unsigned int base, const char *alph, const char *z1,
                                 const char *z2) {
    size_t z1_len = strlen(z1);
    size_t z2_len = strlen(z2);

    if (z1_len < z2_len) {
        return -1;
    } else if (z1_len > z2_len) {
        return 1;
    } else {  // same number of digits

        unsigned char lut[UCHAR_MAX + 1];
        generate_lut(lut, base, alph);

        do {
            if (lut[(unsigned char) *z1] < lut[(unsigned char) *z2]) {
                return -1;
            } else if (lut[(unsigned char) *z1] > lut[(unsigned char) *z2]) {
                return 1;
            }
            z1++;
            z2++;
        } while (*z1 != '\0');
        return 0;
    }
}

/**
 * @brief Subtract two unsigned numbers
 *
 * It is not mandatory for z1 to be bigger than z2.
 *
 * Apart from that this function behaves like add_sub_unsigned(bool, bool, int, const char*,
 * const char*, const char*, char*)
 *
 * @see add_sub_unsigned
 *
 * @param base      The base
 * @param alph      The alphabet
 * @param z1        The first number (minuend)
 * @param z2        The second number (subtrahend)
 * @param result    The result buffer
 */
static void sub_unsigned_to_signed(int base, const char *alph, const char *z1, const char *z2,
                                   char *result) {
    if (cmp_unsigned_pos_base(base, alph, z1, z2) < 0) {
        // -(z2 - z1)
        add_sub_unsigned(false, true, base, alph, z2, z1, result);
    } else {
        // z1 - z2
        add_sub_unsigned(false, false, base, alph, z1, z2, result);
    }
}

/**
 * @brief Multiply an unsigned number by a digit and leftshift the result
 *
 * This method multiplies z1 by the digit z2.
 * The result is shifted to the left by the specified amount of digits.
 * The NULL terminated result will be written to the given buffer.
 *
 * @param shift     The amount of digits for the result to be shifted to the left
 * @param base      The base
 * @param alph      The alphabet
 * @param z1        The first number (multiplicand)
 * @param z2        The second number (multiplier)
 * @param result    The result buffer
 */
static void mul_unsigned_and_shift_left(const unsigned int shift, int base, const char *alph,
                                        const char *z1, const char z2, char *result) {
    char carry_offset = base < 0 ? -1 : 1;

    unsigned char lut[UCHAR_MAX + 1];

    unsigned int base_abs = abs(base);

    // populate lookup table (lut[digit] == index of digit in alphabet)
    generate_lut(lut, base_abs, alph);

    size_t a_len = strlen(z1);

    const char *a_ms_digit = z1;

    // initialize current digit with pointer to last digit
    const char *current_digit_a = a_ms_digit + a_len - 1;

    // write result backwards
    char *current_digit_result = result;

    char b = z2;
    unsigned char b_idx = lut[(unsigned char) b];

    int carry = 0;

    // add with carry as long as there is a digit in a and b
    while (current_digit_a >= a_ms_digit || carry != 0) {
        char a;

        if (current_digit_a < a_ms_digit) {
            a = alph[0];
        } else {
            a = *current_digit_a;
        }

        unsigned char a_idx = lut[(unsigned char) a];

        long prod_idx = a_idx * b_idx + carry;

        if (prod_idx >= base_abs) {
            // carryAdd

            // calculate the resulting digit and carry:
            // * subtract abs(base) from the result until it is a valid digit
            // * add the carry offset to 0 equally often
            carry = 0;
            while (prod_idx >= base_abs) {
                prod_idx -= base_abs;
                carry += carry_offset;
            }

            *current_digit_result = alph[prod_idx];  // 9 * 9 -> 81 -> 1
        } else if (prod_idx < 0) {
            // carrySub

            // calculate the resulting digit and carry:
            // * add abs(base) to the result until it is a valid digit
            // * subtract the carry offset from 0 equally often
            carry = 0;
            while (prod_idx < 0) {
                prod_idx += base_abs;
                carry -= carry_offset;
            }

            *current_digit_result = alph[prod_idx];
        } else {
            // no carry
            *current_digit_result = alph[prod_idx];
            carry = 0;
        }

        current_digit_a--;
        current_digit_result++;
    }

    // strip leading zeros
    while (current_digit_result > (result + 1) && *(current_digit_result - 1) == alph[0]) {
        current_digit_result--;
    }

    // terminate string with NULL byte
    *current_digit_result = '\0';

    // reverse string
    size_t len = strlen(result);
    for (size_t i = 0, j = len - 1; i < j; i++, j--) {
        char tmp = result[i];
        result[i] = result[j];
        result[j] = tmp;
    }

    // shift the result to the left by appending zeros (represented by alph[0])
    for (size_t i = 0; i < shift; i++) {
        *current_digit_result = alph[0];
        current_digit_result++;
    }

    // terminate string with NULL byte
    *current_digit_result = '\0';
}

/**
 * @brief Multiply two unsigned numbers
 *
 * This method multiplies z1 and z2 with the long multiplication algorithm by iterating over the
 * digits of z2. The NULL terminated result will be written to the given buffer. If specified the
 * result will be prefixed by a '-' character.
 *
 * @param negate    True if the result shall be prefixed by '-'
 * @param base      The base
 * @param alph      The alphabet
 * @param z1        The first number (multiplicand)
 * @param z2        The second number (multiplier)
 * @param result    The result buffer
 */
static void mul_unsigned(bool negate, int base, const char *alph, const char *z1, const char *z2,
                         char *result) {
    size_t buffer_size = max_needed_chars_mul(z1, z2) + 1;
    char *tmp_result_1 = malloc(buffer_size);
    check_alloc(tmp_result_1, buffer_size, "temporary result buffer 1");

    char *tmp_result_2 = malloc(buffer_size);
    check_alloc(tmp_result_2, buffer_size, "temporary result buffer 2");

    char *tmp_result_3 = malloc(buffer_size);
    check_alloc(tmp_result_3, buffer_size, "temporary result buffer 3");

    // initialize buffer 1 with 0
    const char zero[2] = {alph[0], '\0'};
    strcpy(tmp_result_1, zero);

    size_t z1_len = strlen(z1);
    size_t z2_len = strlen(z2);

    // pointer to a
    const char *a;
    // pointer to current digit of b (initialized with the least significant digit of b)
    const char *b;
    size_t max_shift;

    // It is slightly more efficient to use the smaller factor as the multiplier because of the
    // accumulated overhead of repeatedly calling mul_unsigned_and_shift_left() and
    // add_sub_unsigned()

    // Therefore we swap z1 and z2 if z2 > z1
    if (z2_len <= z1_len) {
        a = z1;
        max_shift = z2_len - 1;
        b = z2 + max_shift;
    } else {
        a = z2;
        max_shift = z1_len - 1;
        b = z1 + max_shift;
    }

    // iterate over all but the most significant digit from right to left
    for (size_t i = 0; i < max_shift; i++) {
        // multiply a with digit, shift and store result in buffer 2
        mul_unsigned_and_shift_left(i, base, alph, a, *(b - i), tmp_result_2);

        // add buffer 1 and buffer 2 and store result in buffer 3
        add_sub_unsigned(true, false, base, alph, tmp_result_1, tmp_result_2, tmp_result_3);

        // swap buffer 1 and 3
        char *tmp = tmp_result_1;
        tmp_result_1 = tmp_result_3;
        tmp_result_3 = tmp;
    }

    // multiply a with the most significant digit, shift and store result in buffer 2
    mul_unsigned_and_shift_left(max_shift, base, alph, a, *(b - max_shift), tmp_result_2);

    // add buffer 1 and buffer 2, negate if necessary and store result in the result buffer
    add_sub_unsigned(true, negate, base, alph, tmp_result_1, tmp_result_2, result);

    free(tmp_result_1);
    free(tmp_result_2);
    free(tmp_result_3);
}

/**
 * @brief Strip sign and return true if sign is positive
 *
 * @param z A pointer to a pointer to the first digit of the number
 * @return  false if the number is strict negative, else true
 */
static bool strip_sign(const char **z) {
    if (**z == '-') {
        *z = *z + 1;
        return false;
    } else {
        return true;
    }
}

/**
 * @brief Strip leading zeroes
 *
 * @param z A pointer to a pointer to the first digit of the number
 */
static void strip_zeroes(const char **z, char zero) {
    while (**z == zero && *((*z) + 1) != '\0') {
        *z = *z + 1;
    }
}

void impl_naive(int base, const char *alph, const char *z1, const char *z2, char op, char *result) {
    if (base < 0) {
        strip_zeroes(&z1, *alph);
        strip_zeroes(&z2, *alph);

        // since numbers with negative base do not have an explicit sign they can be treated as
        // unsigned numbers
        switch (op) {
            case '+':
                // a + b
                add_sub_unsigned(true, false, base, alph, z1, z2, result);
                break;
            case '-':
                // a - b
                add_sub_unsigned(false, false, base, alph, z1, z2, result);
                break;
            case '*':
                // a * b
                mul_unsigned(false, base, alph, z1, z2, result);
                break;
            default:
                // illegal operator
                return;
        }
    } else {
        // for potentially negative numbers the expression can not be evaluated solely by unsigned
        // operations the following logic converts the expression to make evaluation by using
        // unsigned operations possible

        bool z1_pos = strip_sign(&z1);
        bool z2_pos = strip_sign(&z2);

        strip_zeroes(&z1, *alph);
        strip_zeroes(&z2, *alph);

        switch (op) {
            case '+':
                if (z1_pos && z2_pos) {  // +a + +b
                    // a + b
                    add_sub_unsigned(true, false, base, alph, z1, z2, result);
                } else if (z1_pos) {  // +a + -b
                    // a - b
                    sub_unsigned_to_signed(base, alph, z1, z2, result);
                } else if (z2_pos) {  // -a + +b
                    // b - a
                    sub_unsigned_to_signed(base, alph, z2, z1, result);
                } else {  // -a + -b
                    // -(a + b)
                    add_sub_unsigned(true, true, base, alph, z1, z2, result);
                }
                break;
            case '-':
                if (z1_pos && z2_pos) {  // +a - +b
                    // a - b
                    sub_unsigned_to_signed(base, alph, z1, z2, result);
                } else if (z1_pos) {  // +a - -b
                    // a + b
                    add_sub_unsigned(true, false, base, alph, z1, z2, result);
                } else if (z2_pos) {  // -a - +b
                    // -(a + b)
                    add_sub_unsigned(true, true, base, alph, z1, z2, result);
                } else {  // -a - -b
                    // b - a
                    sub_unsigned_to_signed(base, alph, z2, z1, result);
                }
                break;
            case '*':
                if (z1_pos != z2_pos) {  // +a * -b || -a * +b
                    // -(a * b)
                    mul_unsigned(true, base, alph, z1, z2, result);
                } else {  // -a * -b || +a * +b
                    // a * b
                    mul_unsigned(false, base, alph, z1, z2, result);
                }
                break;
            default:
                // illegal operator
                return;
        }
    }
}

typedef struct {
    char *alph;
    unsigned char *lut;
    char actual;
} Env;

static bool test_exec(Env *env, const char *digit) {
    char actual = env->alph[env->lut[(unsigned char) *digit]];
    env->actual = actual;
    return actual == *digit;
}

static TestResult test_lut(Implementation impl) {
    TestResult tr = test_init_impl(impl, "impl_tests_test lookup table generation");

    unsigned char lut[UCHAR_MAX + 1];

    char *alph = "0123456789";
    int alph_len = 10;
    generate_lut(lut, alph_len, alph);

    Env env = {alph, lut, '\0'};

    for (char *digit = alph; *digit != '\0'; digit++) {
        test_run_with_env(
                &env, digit, (void *) test_exec, &tr, "generate_lut(char[UCHAR_MAX], %i, \"%s\")",
                "alph[lut[(unsigned char) \'%c\']] == '%c'", alph_len, alph, *digit, env.actual);
    }

    test_finalize(tr);

    return tr;
}

void impl_naive_test(Implementation impl) { test_lut(impl); }