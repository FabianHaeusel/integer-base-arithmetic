#include "big_integer_arithmetic.h"

#include <smmintrin.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../../util.h"
#include "arithmetic_helper.h"
#include "logger.h"

static void big_integer_addition_sisd(big_integer *value_a, big_integer *value_b);

static void big_integer_addition_simd(big_integer *value_a, big_integer *value_b);

static void big_integer_subtraction_sisd(big_integer *value_a, big_integer *value_b);

static void big_integer_subtraction_simd(big_integer *value_a, big_integer *value_b);

/**
 * Adds two big integers and stores the result in the first operand. (In-Place addition)
 * Make sure that the first operand can hold the whole result value beforehand! (Otherwise the value
 * will not be calculated entirely).
 * @param value_a The first operand big_integer where the result gets stored after this function.
 * @param value_b The second operand big_integer which gets added to the first one.
 */
void big_integer_addition(big_integer *value_a, big_integer *value_b, bool simd) {
    bool a_sign = value_a->sign;
    bool b_sign = value_b->sign;

    // Some intelligent design choices on adding/subtracting positive/negative values
    // (The idea is to make every sign positive before calculating, because we cannot handle
    // negative numbers with arbitrary sized integers) (Addition/subtraction is here only defined on
    // positive numbers)
    if (a_sign && !b_sign) {
        // a negative; b positive (-a + b) => - (a - b)
        value_a->sign = false;
        big_integer_subtraction(value_a, value_b, simd);

        // change sign of result (invert sign)
        value_a->sign = !value_a->sign;
        return;

    } else if (!a_sign && b_sign) {
        // a positive; b negative (a + -b) => a - b
        value_b->sign = false;
        big_integer_subtraction(value_a, value_b, simd);

        // restore b sign
        value_b->sign = true;
        return;
    }

    // -a + (-b) = -a - b = -(a + b):
    // (if both numbers negative the operation can just be executed like a normal addition, the sign
    // of the result stays)

    // call SIMD or SISD function
    if (simd) {
        big_integer_addition_simd(value_a, value_b);
    } else {
        big_integer_addition_sisd(value_a, value_b);
    }
}

static void big_integer_addition_sisd(big_integer *value_a, big_integer *value_b) {
    // Assuming both numbers are positive or both numbers are negative -> now add them together
    size_t a_length = value_a->length;  // number of bytes in the big_integer
    size_t b_length = value_b->length;  // number of bytes in the big_integer

    bool carry = 0;  // carry from byte to byte
    for (size_t i = 0; i < a_length; i++) {
        // pointer to i-th byte values of big_integers a and b
        uint8_t *a_byte = &(value_a->mem)[i];
        uint16_t a = *(a_byte);

        // result byte
        uint16_t res;
        if (i < b_length) {
            // calculate a_byte + b_byte
            uint8_t *b_byte = &(value_b->mem)[i];
            uint16_t b = *(b_byte);

            res = a + b + carry;

            carry = (res >> 8) & 0x1;  // Carry bit in upper half of 16-bit integer

        } else {
            // if a is longer than b, then still add carry to a
            res = a + carry;
            carry = (res >> 8) & 0x1;
        }

        // assign result back to byte in a
        *(a_byte) = res;
    }

    if (carry == 1) {
        warn("[Binary Addition: SISD] An Overflow occurred while adding two big integers!");
    }
}

static void big_integer_addition_simd(big_integer *value_a, big_integer *value_b) {
    // Assuming both numbers are positive or both numbers are negative -> now add them together
    size_t a_length = value_a->length;  // number of bytes in the big_integer
    size_t b_length = value_b->length;  // number of bytes in the big_integer

    bool carry = 0;  // carry from byte to byte
    size_t i = 0;

    // 15 bytes at a time (SIMD 120 bit)
    for (; (i + 14) < a_length && (i + 14) < b_length; i += 15) {
        __m128i a_bytes = get_15_bytes__of_big_integer(value_a, i);

        __m128i b_bytes = get_15_bytes__of_big_integer(value_b, i);

        // add two 128-bit integers (not directly supported in one function)
        // => add packed 64-bit integers and do carry manually

        // first add two lower halfs (64bit) and previous carry
        __m128i lower64_mask = _mm_set_epi64x(0, 0xFFFFFFFFFFFFFFFFULL);

        __m128i a_lower = _mm_and_si128(a_bytes, lower64_mask);
        __m128i b_lower = _mm_and_si128(b_bytes, lower64_mask);

        // this is true when a_lower is the maximum value (used later for additional carry check)
        bool a_lower_max = (bool) _mm_extract_epi8(_mm_cmpeq_epi64(a_lower, lower64_mask), 0);
        bool b_lower_max = (bool) _mm_extract_epi8(_mm_cmpeq_epi64(b_lower, lower64_mask), 0);

        // previous carry as a 128-bit number
        __m128i carry_128 = _mm_set_epi32(0, 0, 0, (int) carry);

        __m128i result = _mm_add_epi64(a_bytes, b_bytes);
        result = _mm_add_epi64(result, carry_128);

        // if result is less than a or b => carry
        // subtract 2^63 from lower_result, a_lower and b_lower
        __m128i sub_mask = _mm_set_epi64x(0, 9223372036854775808ULL);

        // convert unsigned values to signed values for comparison
        __m128i a_lower_signed = _mm_sub_epi64(a_lower, sub_mask);
        __m128i b_lower_signed = _mm_sub_epi64(b_lower, sub_mask);

        __m128i first_result_lower = _mm_and_si128(result, lower64_mask);
        __m128i lower_result_signed = _mm_sub_epi64(first_result_lower, sub_mask);

        __m128i cmp_a = _mm_cmpgt_epi64(a_lower_signed, lower_result_signed);
        __m128i cmp_b = _mm_cmpgt_epi64(b_lower_signed, lower_result_signed);

        // has lower half set to all ones when a carry happens
        __m128i cmp_ab = _mm_or_si128(cmp_a, cmp_b);

        bool std_carry = !((bool) (_mm_testz_si128(cmp_ab, cmp_ab)));
        bool max_carry = a_lower_max && b_lower_max && carry;
        bool carry_happens = std_carry || max_carry;

        // if a carry happens, add 1 to the higher 64-bit of the result
        if (carry_happens) {
            __m128i higher_1 = _mm_set_epi64x(1, 0);
            result = _mm_add_epi64(result, higher_1);
        }

        // get 0th byte (from the left) to extract carry bit
        uint16_t highest_bytes = _mm_extract_epi16(result, 7);
        carry = (highest_bytes >> 8) & 0x01;

        // write result back into a
        set_15_bytes__of_big_integer(value_a, i, result);
    }
    // 7 bytes at a time (SIMD 56 bit)
    for (; (i + 6) < a_length && (i + 6) < b_length; i += 7) {
        uint64_t a_qword = get_7_bytes__of_big_integer(value_a, i);
        uint64_t b_qword = get_7_bytes__of_big_integer(value_b, i);

        uint64_t sum = a_qword + b_qword;
        sum += carry;
        // get 0th byte (from the left) to extract carry bit
        carry = (sum >> 56) & 0x1;

        // write result back into a
        set_7_bytes__of_big_integer(value_a, i, sum);
    }

    for (; i < a_length; i++) {
        // pointer to i-th byte values of big_integers a and b
        uint8_t *a_byte = &(value_a->mem)[i];
        uint16_t a = *(a_byte);

        // result byte
        uint16_t res;
        if (i < b_length) {
            // calculate a_byte + b_byte
            uint8_t *b_byte = &(value_b->mem)[i];
            uint16_t b = *(b_byte);

            res = a + b + carry;
            carry = (res >> 8) & 0x1;  // Carry bit in upper half of 16-bit integer

        } else {
            // if a is longer than b, then still add carry to a
            res = a + carry;
            carry = (res >> 8) & 0x1;
        }

        //assign result back to byte in a
        *(a_byte) = res;
    }

    if (carry == 1) {
        warn("[Binary Addition: SIMD] An Overflow occurred while adding two big integers!");
    }
}

/**
 * Subtracts the second value (b) from the first value (a) and stores the result in the first value
 * (a).
 * @param value_a The first operand big_integer where the result gets stored to.
 * @param value_b The second operand big_integer which gets subtracted from the first operand.
 */
void big_integer_subtraction(big_integer *value_a, big_integer *value_b, bool simd) {
    bool a_sign = value_a->sign;
    bool b_sign = value_b->sign;

    if (!a_sign && b_sign) {
        // a positive; b negative (a - -b) => a + b
        value_b->sign = false;
        big_integer_addition(value_a, value_b, simd);

        // restore sign of value b
        value_b->sign = true;
        return;

    } else if (a_sign && !b_sign) {
        // a negative; b positive (-a - b) => -(a + b)
        value_a->sign = false;
        big_integer_addition(value_a, value_b, simd);

        // negate result
        negate_big_integer(value_a);
        return;
    } else if (a_sign && b_sign) {
        //-a - (-b) = -a + b = b - a

        // make a,b positive for calculation
        value_a->sign = false;

        // make sure that b is big enough to hold the value
        big_integer *b_copy =
                clone_big_integer_add_size(value_b, max(0, value_a->length - value_b->length));
        b_copy->sign = false;

        // b - a
        big_integer_subtraction(b_copy, value_a, simd);

        // move result into a
        copy_big_integer_value_into_another(b_copy, value_a);

        delete_big_integer(b_copy);
        return;
    }

    // subtracting two positive values from each other

    /*
     * special case:
     * a - b where a < b a:small, b:big
     * => a - b = -b + a = -(b - a)
     *
     */
    // check if b is greater than a
    if (positive_big_integer_is_greater_than(value_b, value_a, simd)) {
        // Save the old value b that will be overwritten by the subtraction method
        big_integer *b_copy = clone_big_integer(value_b);

        // subtract a from b (because b is big and a is small)
        big_integer_subtraction(value_b, value_a, simd);  // big - small

        // negate the sign of the result
        negate_big_integer(value_b);

        // set result to original a big_integer (value_b -> value_a)
        copy_big_integer_value_into_another(value_b, value_a);
        // restore old value of b (b_copy -> value_b)
        copy_big_integer_value_into_another(b_copy, value_b);

        delete_big_integer(b_copy);
        return;
    }

    // Choose either SISD or SIMD implementation
    if (simd) {
        big_integer_subtraction_simd(value_a, value_b);
    } else {
        big_integer_subtraction_sisd(value_a, value_b);
    }
}

/**
 * Subtracts two big_integers from each other and stores the result in the first operand.
 * Note: This function needs the requirement that both values have the same sign and abs(a) is
 * greater than abs(b). Therefore it should not be called outside of this file.
 */
static void big_integer_subtraction_sisd(big_integer *value_a, big_integer *value_b) {
    // Assuming a is greater than b and a and b are either both positive or both negative

    size_t a_length = value_a->length;  // number of bytes in the big_integer a
    size_t b_length = value_b->length;  // number of bytes in the big_integer b

    bool borrow = 0;  // carry from byte to byte
    for (size_t i = 0; i < a_length; i++) {
        // pointer to i-th byte values of big_integers a and b
        uint8_t *a_byte = ((uint8_t *) value_a->mem) + i;
        uint16_t a = *a_byte;

        uint16_t res;
        if (i < b_length) {
            uint8_t *b_byte = ((uint8_t *) value_b->mem) + i;
            uint16_t b = *b_byte;

            // calculate sum of bytes and carry
            res = a - b - borrow;
            borrow = (res >> 15) & 0x1;  // Borrow when result is negative => get sign bit of result
        } else {
            // if a is longer than b, we still need to subtract the remaining borrow from a
            res = a - borrow;
            borrow = (res >> 15) & 0x1;
        }

        // assign result back to byte in a
        *((uint8_t *) a_byte) = res;
    }

    if (borrow == 1) {
        warn("[Binary Subtraction SISD] An Overflow occurred while subtracting two big integers!");
    }
}

/**
 * SIMD-Optimization of big_integer subtraction.
 * First 15 bytes at a time, then 7 bytes at a time, then 1 byte at a time.
 */
static void big_integer_subtraction_simd(big_integer *value_a, big_integer *value_b) {
    // Assuming a is greater than b and a and b are either both positive or both negative

    size_t a_length = value_a->length;  // number of bytes in the big_integer a
    size_t b_length = value_b->length;  // number of bytes in the big_integer b

    bool borrow = 0;  // carry from byte to byte

    // 15 bytes at a time (SIMD 120 bit)
    size_t i = 0;
    for (; (i + 14) < a_length && (i + 14) < b_length; i += 15) {
        __m128i a_bytes = get_15_bytes__of_big_integer(value_a, i);
        __m128i b_bytes = get_15_bytes__of_big_integer(value_b, i);

        // add two 128-bit integers (not directly supported in one function)
        // => add packed 64-bit integers and do carry manually

        // first add two lower halfs (64bit) and previous carry
        __m128i lower64_mask = _mm_set_epi64x(0, 0xFFFFFFFFFFFFFFFFULL);

        __m128i a_lower = _mm_and_si128(a_bytes, lower64_mask);
        __m128i b_lower = _mm_and_si128(b_bytes, lower64_mask);

        // this is true when b_lower is the maximum value (used later for additional carry check)
        bool b_lower_max = (bool) _mm_extract_epi8(_mm_cmpeq_epi64(b_lower, lower64_mask), 0);

        // previous borrow as a 128-bit number
        __m128i borrow_128 = _mm_set_epi32(0, 0, 0, (int) borrow);

        __m128i result = _mm_sub_epi64(a_bytes, b_bytes);
        result = _mm_sub_epi64(result, borrow_128);

        // if result is greater than a => borrow
        // subtract 2^63 from lower_result, a_lower and b_lower
        __m128i sub_mask = _mm_set_epi64x(0, 9223372036854775808ULL);

        // convert unsigned values to signed values for comparison
        __m128i a_lower_signed = _mm_sub_epi64(a_lower, sub_mask);

        __m128i first_result_lower = _mm_and_si128(result, lower64_mask);
        __m128i lower_result_signed = _mm_sub_epi64(first_result_lower, sub_mask);

        // has lower half set to all ones when a borrow happens
        __m128i cmp_a = _mm_cmpgt_epi64(lower_result_signed, a_lower_signed);

        bool std_borrow = !((bool) (_mm_testz_si128(cmp_a, cmp_a)));
        bool max_borrow = b_lower_max && borrow == 1;
        bool borrow_happens = std_borrow || max_borrow;

        // if a borrow happens, add 1 to the higher 64-bit of the result
        // borrow happens if: res(unsigned) > a || res(unsigned) > b || (b == max) && borrow_in
        if (borrow_happens) {
            // sub 1 from higher 56 bit
            __m128i higher_1 = _mm_set_epi64x(1, 0);
            result = _mm_sub_epi64(result, higher_1);
        }
        // get 0th byte (from the left) to extract borrow bit
        uint16_t highest_bytes = _mm_extract_epi16(result, 7);
        borrow = (highest_bytes >> 15) & 0x01;

        // write result back into a
        set_15_bytes__of_big_integer(value_a, i, result);
    }

    // 7 bytes at a time (SIMD 56 bit)
    for (; (i + 6) < a_length && (i + 6) < b_length; i += 7) {
        uint64_t a_qword = get_7_bytes__of_big_integer(value_a, i);
        uint64_t b_qword = get_7_bytes__of_big_integer(value_b, i);

        uint64_t res = a_qword - b_qword;
        res -= borrow;
        // get 0th byte (from the left) to extract carry bit
        borrow = (res >> 56) & 0x1;

        // write result back into a
        set_7_bytes__of_big_integer(value_a, i, res);
    }

    // 1 byte at a time (byte-wise SISD)
    for (; i < a_length; i++) {
        // pointer to i-th byte values of big_integers a and b
        uint8_t *a_byte = ((uint8_t *) value_a->mem) + i;
        uint16_t a = *a_byte;

        uint16_t res;
        if (i < b_length) {
            uint8_t *b_byte = ((uint8_t *) value_b->mem) + i;
            uint16_t b = *b_byte;

            // calculate sum of bytes and carry
            res = a - b - borrow;
            borrow = (res >> 15) & 0x1;  // Borrow when result is negative => get sign bit of result
        } else {
            // if a is longer than b, we still need to subtract the remaining borrow from a
            res = a - borrow;
            borrow = (res >> 15) & 0x1;
        }

        // assign result back to byte in a
        *((uint8_t *) a_byte) = res;
    }

    if (borrow == 1) {
        warn("[Binary Subtraction SIMD] An Underflow occurred while subtracting two big integers!");
    }
}

/**
 * Increments the given big_integer by 1.
 * Warning: no overflow/underflow checks.
 */
void big_integer_increment(big_integer *value) {
    bool sign = value->sign;
    for (size_t i = 0; i < value->length; i++) {
        // when positive: +1
        // when negative: -1
        uint8_t byte = get_byte_value_of_big_integer(value, i);
        if (!sign) {
            uint16_t inc = byte + 1;
            set_byte_value_of_big_integer(value, i, inc);
            // carry
            bool carry = inc >> 8;
            if (carry == 0) {
                break;  // done
            }           // when carry, go to next byte...
        } else {
            if (byte == 0) continue;
            set_byte_value_of_big_integer(value, i, byte - 1);
        }
    }
}

/**
 * Shifts the given big_integer left by count bits (in-place). count is limited to [0;7]
 * If the provided big_integer is not big enough to hold the shifted result, the remaining
 * higher-significant bits will get cut.
 * @param value The big_integer to shift.
 * @param bit_count The amount of bits the integer should be shifted to the left (direction of most
 * significant bit). The only valid values are in range [0;7]
 * @param result The pointer to the big_integer where the result should be stored.
 */
void big_integer_shl_bitwise_0_to_7(big_integer *value, uint8_t bit_count, bool simd) {
    if (simd)
        big_integer_shl_bitwise_0_to_7__simd56(value, bit_count);
    else
        big_integer_shl_bitwise_0_to_7__sisd(value, bit_count);
}

void big_integer_shl_bitwise_0_to_7__sisd(big_integer *value, uint8_t bit_count) {
    int length = (int) value->length;

    // one byte at a time (SISD)
    uint8_t carry = 0;
    for (int i = 0; i < length; i++) {
        uint8_t byte = get_byte_value_of_big_integer(value, i);
        uint16_t shifted = (uint16_t) byte << bit_count;

        // add shifted_out (upper half from last iteration)
        shifted |= carry;

        // write lower byte back to mem
        uint8_t lower_byte = (uint8_t) shifted;
        set_byte_value_of_big_integer(value, i, lower_byte);

        // save upper half for next iteration
        carry = (uint8_t) (shifted >> 8);  // (upper half)
    }
}

void big_integer_shl_bitwise_0_to_7__simd56(big_integer *value, uint8_t bit_count) {
    int length = (int) value->length;

    // 7 bytes at a time (SIMD 56)
    // we put 7 bytes into 8 byte registers and then shift by [0;7], so we only need one byte more
    uint8_t carry = 0;
    int i = 0;
    for (; (i + 6) < length; i += 7) {
        uint64_t value7 = get_7_bytes__of_big_integer(value, i);
        uint64_t shifted = value7 << bit_count;
        shifted |= carry;
        // lower 7 bytes
        uint64_t lower7 = shifted & 0x00FFFFFFFFFFFFFF;
        set_7_bytes__of_big_integer(value, i, lower7);
        // set carry
        carry = shifted >> 56;
    }

    // last remaining bytes, one byte at a time (SISD)
    for (; i < length; i++) {
        uint8_t byte = get_byte_value_of_big_integer(value, i);
        uint16_t shifted = (uint16_t) byte << bit_count;

        // add shifted_out (upper half from last iteration)
        shifted |= carry;

        // write lower byte back to mem
        uint8_t lower_byte = (uint8_t) shifted;
        set_byte_value_of_big_integer(value, i, lower_byte);

        // save upper half for next iteration
        carry = (uint8_t) (shifted >> 8);  // (upper half)
    }
}

/**
 * Shifts the given big_integer by count bytes to the left (in-place).
 * Make sure that the big_integer is big enough to hold the new value!
 * @param value The big_integer value.
 * @param count The number of bytes the big_integer should be shifted to left.
 */
void big_integer_shl_byte_wise(big_integer *value, size_t count) {
    // Copy the values from 0 to length-2 from the original value one byte up
    // because the first <count> bytes should be set to zero.

    for (int i = value->length - count - 1; i >= 0; i--) {
        // length 8, shl 2: 8 - 2 - 1 = 5
        set_byte_value_of_big_integer(value, i + count, get_byte_value_of_big_integer(value, i));
    }
    for (size_t i = 0; i < count; i++) {
        set_byte_value_of_big_integer(value, i, 0);
    }
}

/**
 * Multiplies a big_integer by the given 8 bit unsigned value and returns the big_integer containing
 * the result. (This is used for multiplying the weight of a digit with its digit value (which is
 * between 0 and incl. 255).
 * @param value The big_integer value to be multiplied. This must not be the same big_integer as
 * result!
 * @param mul The factor with which the first factor gets multiplied.
 * @param result The big_integer in which the result of the multiplication will be written to.
 * @param temp A big_integer that is big enough to store the value + 1 byte (!), that can be used as
 * a temporary value (caller-saved)
 * @param simd Flag if simd operations should be used or not.
 * The result of a big_integer multiplied by one byte fits into a big_integer of length of value + 1
 */
void big_integer_multiply_uint8(big_integer *value, uint8_t mul, big_integer *result,
                                big_integer *temp, bool simd) {
    // Clear the big_integer where the result will be written into
    set_zero(result);

    // Partial product is initialized as the copy of the value with 1 byte more space
    // because the left bit-shift can otherwise shift the highest bits out of bounds.
    copy_big_integer_value_into_another(value, temp);

    int to_shift = 0;
    // Multiply with uint8
    for (int i = 0; i < 8; i++) {
        bool bit = (mul >> i) & 0x1;  // get i-th bit
        if (bit) {                    // if (bit != 0)

            if (i != 0) {
                // shift partial product left
                big_integer_shl_bitwise_0_to_7(temp, 1 + to_shift, simd);
                to_shift = 0;
            }
            // add partial product to result
            big_integer_addition(result, temp, simd);

        } else if (i != 0) {
            to_shift++;
        }
    }
}

/**
 * Multiplies the big_integers a and b byte-wise and stores the result in the big_integer which
 * pointer is given as argument.
 * @param a First operand a.
 * @param b Second operand b.
 * @param res The big_integer where the result of the multiplication will be written into. Make sure
 * that this is big enough to hold the full value. This function does not check the size.
 * @param simd True when SIMD-implementation should be used.
 */
void big_integer_multiplication(big_integer *a, big_integer *b, big_integer *res, bool simd) {
    size_t b_len = b->length;

    big_integer *pp = create_big_integer(res->length, false);
    big_integer *temp = create_big_integer(a->length + 1, false);

    // Multiply
    // Start significant byte of b
    for (size_t i = 0; i < b_len; i++) {
        uint8_t byte = get_byte_value_of_big_integer(b, i);
        if (byte == 0) continue;

        // Multiply current byte of b with whole of a
        big_integer_multiply_uint8(a, byte, pp, temp, simd);
        big_integer_shl_byte_wise(pp, i);

        big_integer_addition(res, pp, simd);
    }

    delete_big_integer(pp);
    delete_big_integer(temp);

    // Change sign accordingly
    // negative when either: -v * m = -r OR v * -m = -r
    res->sign = (a->sign && !b->sign) || (b->sign && !a->sign);
}

/**
 * Multiply big_integer with signed 16-bit integer only in range [-256;256] (Wider range than radix).
 * (Used for multiplying a multiple of a base with the base itself i.e. calculating the power of a
 * base).
 * @param value The value to be multiplied. Doesn't get changed in this method.
 * @param mul The signed 16-bit integer.
 * @param temp A big_integer that is big enough to store the value that can be used as a temporary
 * value (caller-saved)
 * @param simd Flag if simd operations should be used or not.
 */
void big_integer_multiply_int_neg256_to_256(big_integer *value, int16_t mul, big_integer *result,
                                            big_integer *temp, bool simd) {
    // Call big_integer multiplication uint8 with absolute value of mul (absolute of signed 16-bit
    // is unsigned 8-bit).
    big_integer_multiply_uint8(value, abs(mul), result, temp, simd);

    // If needed, change the sign from the result to negative.
    // either: -v * m = -r OR v * -m = -r
    result->sign = (value->sign && mul > 0) || (!value->sign && mul < 0);
}

/**
 * Divides the big_integer value by divisor (signed 16 bit integer in range [-128; 128]). The
 * big_integer will contain the result of the division.
 * @param value The value to be divided.
 * @param divisor The divisor with which the value gets divided.
 * @param temp_value A temp big_integer that should be big enough to hold the result of the
 * division.
 * @param temp_value2 A temporary big_integer that should be big enough to hold the value of the
 * remainder.
 * @return Returns the remainder (Euclidean mod) of the division of value and divisor.
 */
int16_t big_integer_division_int9_t(big_integer *value, int16_t divisor, big_integer *temp_value,
                                    big_integer *temp_value2, bool simd) {
    // Using naive unsigned binary long division algorithm
    if (divisor == 0) {
        abort_err("[FATAL] big_integer_division_int9_t: Division by zero.");
    }

    // perform unsigned division, change sign and remainder later
    uint8_t div = abs(divisor);
    bool value_sign = value->sign;
    value->sign = false;

    big_integer *quotient = temp_value;
    set_zero(quotient);

    // holds the remainder in the end, temporary values while calculating
    big_integer *remainder = temp_value2;
    set_zero(remainder);

    big_integer *divisor_big = create_big_integer(2, false);  // used for later
    set_byte_value_of_big_integer(divisor_big, 0, abs(divisor));

    // i for every byte in value
    // j for every bit in every byte (Decrementing because we start at the most significant
    // byte/bit)
    for (int i = value->length - 1; i >= 0; i--) {
        for (int j = 7; j >= 0; j--) {
            // shift the remainder left by one
            big_integer_shl_bitwise_0_to_7(remainder, 1, simd);

            // set the lowest bit of the remainder to the current (i-th) bit of the value
            bool bit_i = (get_byte_value_of_big_integer(value, i) >> j) & 0x1;
            set_byte_value_of_big_integer(remainder, 0,
                                          get_byte_value_of_big_integer(remainder, 0) | bit_i);

            bool b = big_integer_greater_equal_int16(remainder, div, simd);
            if (b) {
                // remainder -= divisor
                big_integer_subtraction(remainder, divisor_big, simd);
                set_bit_value_of_big_integer(quotient, i * 8 + j, 1);
            }
        }
    }

    // copy result into value big_integer
    copy_big_integer_value_into_another(quotient, value);
    int16_t remainder16 = get_byte_value_of_big_integer(remainder, 0);

    // change sign and remainder
    if ((value_sign && divisor > 0) || (!value_sign && divisor < 0)) {
        value->sign = true;
    }
    if (value_sign) {
        // the remainder is always positive and only positive when the first operand is negative
        remainder16 *= -1;
    }

    // clear memory
    delete_big_integer(divisor_big);

    return remainder16;
}

/*
 * =====================================================================
 * Big integer comparison
 * =====================================================================
 */

/**
 * Return whether a is greater than b. Only limited to positive big_integers!
 * @param a
 * @param b
 */
bool positive_big_integer_is_greater_than(big_integer *a, big_integer *b, bool simd) {
    if (a->sign || b->sign) {
        abort_err(
                "The function positive_big_integer_is_greater_than is only designed for positive "
                "big_integers!");
    }

    int a_len = (int) a->length;
    int b_len = (int) b->length;
    size_t max_length = max(a_len, b_len);

    bool a_zero = big_integer_is_zero(a, simd);
    bool b_zero = big_integer_is_zero(b, simd);

    if (a_zero && b_zero) return false;  // a = b = 0
    if (a_zero && !b->sign) return false;  // a = 0; b > 0 => a < b

    // going from last (most significant) byte to lowest
    for (int i = (int) max_length - 1; i >= 0; i--) {
        uint8_t a_byte = 0;
        uint8_t b_byte = 0;

        if (i < a_len) {
            a_byte = get_byte_value_of_big_integer(a, i);
        }
        if (i < b_len) {
            b_byte = get_byte_value_of_big_integer(b, i);
        }

        // a/b has higher weight bytes than the other number that are not zero.
        if (i >= b_len && i < a_len) {
            if (a_byte != 0) {
                // a is longer and has non null-bytes -> a is greater than b"
                return true;
            } else continue;
        }
        if (i >= a_len && i < b_len) {
            if (b_byte != 0) {
                // b is longer and has non null-bytes -> b is greater than a"
                return false;
            } else continue;
        }
        if (a_byte == b_byte) {
            continue;
        }
        return a_byte > b_byte;
    }
    return false;
}

/**
 * Returns true iff the given signed big_integer has a greater value than b or an equal value to b.
 * b is a signed 16 bit integer in the range [-256;256].
 * @param a
 * @param b
 */
bool big_integer_greater_equal_int16(big_integer *a, int16_t b, bool simd) {
    size_t len = a->length;
    bool a_negative = a->sign;

    bool a_zero = big_integer_is_zero(a, simd);
    bool b_zero = b == 0;

    // check trivial cases: one/both zero, signs different
    if (a_zero && b_zero) return true;  // a = b = 0

    if (a_zero && b < 0) return true;   // a = 0; b < 0 => a > b
    if (a_zero && b > 0) return false;  // a = 0; b > 0 => a < b

    if (a_negative && b >= 0) return false;  // a < 0 <= b => a < b
    if (!a_negative && b <= 0) return true;  // a > 0 >= b => a > b

    // The following code compares the big_integer with the 9bit signed integer after the following
    // rules:
    /*
     *  fb = first byte of a
     *  both positive:
     *     fb == b: equal (or potentially is a greater than b) => return true
     *     fb > b: a definitely greater than b => return true
     *     fb < b: check for each byte [1;n[, if byte is greater than zero => return true, else
     * after all, return false
     *
     *  both negative:
     *     fb == b: check for each byte [1;n[, if byte is greater than zero => return false, else
     * after all, return true fb > b: a definitely smaller than b => return false fb < b: check for
     * each byte [1;n[, if byte greater than zero => return false, else after all, return true
     *
     */

    // compare first byte
    uint8_t first_byte = get_byte_value_of_big_integer(a, 0);

    // pos 1.
    if (!a_negative && first_byte == b) {
        // positive and first byte equal => a >= b
        return true;
    }
    // pos 2.
    if (!a_negative && first_byte > b) {
        // both positive: a is definitely greater than b
        return true;
    }

    // neg 2.
    if (a_negative && first_byte > b) {
        // negative: a must be smaller than b, a cannot be any bigger now, can only be smaller on
        // higher bytes
        return false;
    }

    // a is not zero
    // signs are the same
    for (size_t i = 1; i < len; i++) {
        uint8_t byte = get_byte_value_of_big_integer(a, i);

        // neg 1., 3.
        if (a_negative && byte > 0) {
            // cannot be bigger anymore/equal than b
            return false;
        }

        // pos 3.
        if (!a_negative && byte > 0) {
            // positive and byte > 0
            return true;
        }
    }

    // pos 3.
    if (!a_negative) return false;
    // neg 1., 3.
    return true;
}
