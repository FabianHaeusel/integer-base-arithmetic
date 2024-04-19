#include "impl_binary_conversion.h"

#include <stdbool.h>
#include <string.h>

#include "../../util.h"
#include "arithmetic_helper.h"
#include "big_integer.h"
#include "big_integer_arithmetic.h"
#include "logger.h"
#include "../common.h"

void arith_op_any_base__binary_conversion__sisd(int base, const char *alph, const char *z1,
                                                const char *z2, char op, char *result) {
    arith_op_any_base__binary_conversion(base, alph, z1, z2, op, result, false);
}

void arith_op_any_base__binary_conversion__simd(int base, const char *alph, const char *z1,
                                                const char *z2, char op, char *result) {
    arith_op_any_base__binary_conversion(base, alph, z1, z2, op, result, true);
}

/**
 * This function first converts both operands (that are encoded in the given base) to binary. Then,
 * it executes the corresponding operation (either +,- or *) and converts the result back to the
 * desired base format and writes in into the given result string. The result string is terminated
 * with a NULL-byte when the number is finished, no matter how long the original number strings
 * were. The caller of this function needs to make sure that the given result string is big enough
 * to hold the output value when calling this function.
 *
 * @param base Whole number in range [-128;128] (except: -1, 0 and 1) which indicates the base/radix
 * of the input/output numbers.
 * @param alph The alphabet that maps each numeric value to a ascii-representable character (digit)
 * @param z1 The operand 1 string encoded in the desired base format.
 * @param z2 The operand 2 string encoded in the desired base format.
 * @param op The operation, either '+', '-' or '*' (respective addition, subtraction or
 * multiplication).
 * @param result The string buffer where the result in encoded format is written to.
 */
void arith_op_any_base__binary_conversion(int base, const char *alph, const char *z1,
                                          const char *z2, char op, char *result, bool simd) {
    // Flag to set whether SIMD implementations of the functions should be used or if not (then the
    // standard SISD implementations will be executed, which usually only process one byte at a
    // time)

    // Note: negative numbers with in positive base systems have a leading '-' char
    //      in negative bases, there is no need for a '-' char because negative values are encoded
    //      already.

    // Step 1: Conversion of operands to binary
    // This bool indicates of the result of the conversion must be later negated (because the
    // "string" number is neg.).
    bool z1_negative = base > 1 && z1[0] == '-';
    bool z2_negative = base > 1 && z2[0] == '-';

    size_t z1_length = strlen(z1);
    size_t z2_length = strlen(z2);
    size_t result_length;

    // Create big_integer elements that later hold the converted values of z1 and z2
    size_t z1_binary_minsize = get_big_integer_min_size((int16_t) base, z1_length);
    size_t z2_binary_minsize = get_big_integer_min_size((int16_t) base, z2_length);

    big_integer *z1_binary, *z2_binary = create_big_integer(z2_binary_minsize, false);

    // if addition or subtraction, z1 will hold the result and must therefore have the minimum
    // desired size
    size_t addition_subtraction_min_result_size = max(z1_binary_minsize, z2_binary_minsize) + 1;
    if (op == '+' || op == '-') {
        z1_binary = create_big_integer(addition_subtraction_min_result_size, false);
    } else {
        // multiplication
        z1_binary = create_big_integer(z1_binary_minsize, false);
    }

    // Convert string numbers into binary
    convert_numbers_from_any_base_into_binary(base, alph, z1, z2, z1_length, z2_length, z1_binary,
                                              z2_binary, simd);

    // add sign if base is positive and first char of number is a '-'
    if (z1_negative) z1_binary->sign = true;
    if (z2_negative) z2_binary->sign = true;

    // Step 2: Perform the actual arithmetic operation on the converted binary values.
    big_integer *res;
    switch (op) {
        case '+':
            result_length = max(z1_length, z2_length) + 2;
            if (base < 0) {
                result_length += 1;
            }
            big_integer_addition(z1_binary, z2_binary, simd);
            res = z1_binary;
            // Addition/Subtraction: Result length just digits is max(a,b) + 1, then add 2 because
            // of sign and NULL-byte
            break;
        case '-':
            big_integer_subtraction(z1_binary, z2_binary, simd);
            res = z1_binary;
            result_length = max(z1_length, z2_length) + 3;
            break;
        case '*':
            res = create_big_integer(z1_binary->length + z2_binary->length, false);

            big_integer_multiplication(z1_binary, z2_binary, res, simd);
            result_length = max(z1_length, z2_length) * 2 + 1;
            // Clear z1 separately (because in addition/subtraction, res refers to z1 and will be
            // deleted after conversion)
            delete_big_integer(z1_binary);
            break;
        default:
            abort_err("The provided operation %c is not valid!", op);
    }

    // change sign accordingly
    if (big_integer_is_zero(res, simd)) {
        res->sign = false;
    }

    // Step 3: Convert the result back to the original base and write it to the given buffer.
    convert_big_integer_to_any_base(res, base, alph, result, result_length, simd);

    // Clear memory
    delete_big_integer(res);
    delete_big_integer(z2_binary);
}

/**
 * Gets the char weight (value) by the given char and lookup table (array pointer).
 * @param c The char to look up.
 * @param lookup The lookup table, as array pointer, where each char is mapped to its weight (value)
 * in the specific numeric system.
 * @return
 */
uint8_t get_char_value(char c, uint8_t (*lookup)[]) { return (*lookup)[(uint8_t) c]; }

/**
 * Converts the given strings z1, z2 to their binary representation and stores them in the given
 * big_integers by adding each char value with the corresponding weight to one big_integer.
 */
void convert_numbers_from_any_base_into_binary(int base, const char *alph, const char *z1,
                                               const char *z2, size_t z1_length, size_t z2_length,
                                               big_integer *z1_binary, big_integer *z2_binary,
                                               bool simd) {
    // Create a lookup-table to get the values of the characters of the alphabet quickly in
    // calculation.
    uint8_t lookup[UCHAR_MAX + 1] = {};
    generate_lut(lookup, strlen(alph), alph);

    // Big_integer used for calculation that is big enough to hold the final value of z1, z2
    big_integer *z1_temp = create_big_integer(z1_binary->length, false);
    big_integer *z2_temp = create_big_integer(z2_binary->length, false);

    // Create a big_integer than can hold up to the value of base^(max_length - 1), which is the
    // weight the last most significant char in the longest of the input strings. Init that value to
    // 1, which is base^0
    size_t max_length = max(z1_length, z2_length);

    big_integer *current_weight = create_big_integer(
            get_big_integer_min_size_exponentiation((int16_t) base, (int) max_length), false);
    big_integer *temp = create_big_integer(current_weight->length, false);
    big_integer *temp2 = create_big_integer(current_weight->length, false);

    set_byte_value_of_big_integer(current_weight, 0, 1);

    // i: String iterating char-index (Starting at the front of the string)
    for (size_t i = 0; i < max_length; i++) {
        // The index of the digit (starting with n-1 at z1[0] or z2[0]) (Starting at the back of the
        // string)
        if (i < z1_length) {
            size_t char_index = z1_length - 1 - i;
            // digit_value is in between [0; base) but max in [0; 128) because max base is 128
            uint8_t digit_value = get_char_value(z1[char_index], &lookup);

            // The total value of the digit is its digit value multiplied by the weight of the
            // position. Because the current weight is only multiplied by a one-byte value, the
            // total_digit value fits into length of current_weight + 1 bytes.
            big_integer_multiply_uint8(current_weight, digit_value, z1_temp, temp2, simd);

            // Add the total value of the character to the value of the number (z1, z2).
            big_integer_addition(z1_binary, z1_temp, simd);
        }
        if (i < z2_length) {
            size_t char_index = z2_length - 1 - i;
            uint8_t digit_value = get_char_value(z2[char_index], &lookup);

            big_integer_multiply_uint8(current_weight, digit_value, z2_temp, temp2, simd);
            big_integer_addition(z2_binary, z2_temp, simd);
        }

        // Multiply the current_weight with the base to get the base of the next char (to the left).
        big_integer_multiply_int_neg256_to_256(current_weight, (int16_t) base, temp, temp2, simd);

        // swap current_weight and temp
        big_integer *temp_temp = temp;
        temp = current_weight;
        current_weight = temp_temp;
    }

    // Clear temp memory
    delete_big_integer(temp);
    delete_big_integer(temp2);
    delete_big_integer(current_weight);
    delete_big_integer(z1_temp);
    delete_big_integer(z2_temp);
}

/**
 * Converts the given big_integer value to a string (in buffer) that is encoded in the given base
 * with the given alphabet.
 * @param value The value that should be converted.
 * @param base The base in which the value should be converted. Note: The only valid values are
 * in range [-128; 128]!
 * @param alph The string indicating the alphabet of the numeric system of the base.
 * @param buffer The buffer where the output string should be written to.
 * @param buffer_length The size of the output buffer, inclusive NULL-byte!
 */
void convert_big_integer_to_any_base(big_integer *value, int16_t base, const char *alph,
                                     char *buffer, size_t buffer_length, bool simd) {
    // The Double Dabble algorithm is used for positive bases (faster than division).

    if (base > 0) {
        // explanation and examples of the double-dabble algorithm:
        // we need log_base(2^(bits of value)) digits for output
        // rule for base 10: if (nibble >= 5), add 3 then shift (so next bitshift will result in +6,
        // which will effectively carry (10 will become 16 which will become carried 0x1 0x0)
        // => add 6, after shift when greater/equal than 10 (because in odd bases there is no whole
        // number that represents the half of the odd number (256-base) double dabble using one byte
        // for each digit (no matter the base)

        // base 2 (0...1) => before shift: if >= 2: +254
        // base 7 (0...6) => before shift: if >= 4: 4 * 2 + 1 = 9 (!!!)
        //...
        // base 10 (0...9) => after shift: if >= 10: +246
        // base 11 (0...10) => after shift: if >= 11: +245 (12 => 17 => bc11: 0x1 0x1 "=" 14)
        // base 12 (0...11) => after shift: if >= 12: +244                      6 => +2 (6+2 = 8;
        // 8*2 = 16 => bc12: 0x1 0x0 "=" 12) base 13 (0...12) => after shift: if >= 13: +243 (14 =>
        // 17 => bc13: 0x1 0x1 "=" 14) base 14 (0...13) => after shift: if >= 14: +242 base 15
        // (0...14) => after shift: if >= 15: +241 base 16 (0...15) => after shift: if >= 16: +240
        //...
        // base 250 (0...249) => after shift: if >= 250: +6
        // base 251 (0...250) => after shift: if >= 251: +5
        // base 256 (0...255) => after shift: passt (+0)

        uint8_t conversion_trigger = base;
        uint8_t carry_add = 256 - base;

        // the big_integer buffer used for calculation
        big_integer *calc_buffer = create_big_integer(buffer_length, false);
        big_integer *remaining_value = clone_big_integer(value);

        int value_length = value->length;

        // Perform double-dabble iteration for each bit of input value
        for (int i = 0; i < value_length * 8; i++) {
            // 1. double: shift left once
            big_integer_shl_bitwise_0_to_7(calc_buffer, 1, simd);

            // set the least significant bit of calc_buffer as the most significant bit of the input
            // value
            bool most_significant_bit = get_most_significant_bit(remaining_value);
            set_byte_value_of_big_integer(
                    calc_buffer, 0,
                    get_byte_value_of_big_integer(calc_buffer, 0) | most_significant_bit);

            // shift the input value left once
            big_integer_shl_bitwise_0_to_7(remaining_value, 1, simd);

            // 2. dabble: adjust bytes that are greater/equal than/as the base
            for (size_t j = 0; j < calc_buffer->length; j++) {
                uint8_t byte = get_byte_value_of_big_integer(calc_buffer, j);
                if (byte >= conversion_trigger) {
                    set_byte_value_of_big_integer(calc_buffer, j, byte + carry_add);

                    // carry: +1 @ next byte
                    set_byte_value_of_big_integer(
                            calc_buffer, j + 1, get_byte_value_of_big_integer(calc_buffer, j + 1) + 1);
                }
            }
        }

        // get most significant byte of calc_buffer that is not zero
        int calc_buffer_start_nonzero = 0;
        for (int i = calc_buffer->length - 1; i >= 0; i--) {
            if (get_byte_value_of_big_integer(calc_buffer, i) != 0) {
                calc_buffer_start_nonzero = i;
                break;
            }
        }

        // 3. Convert the result into the desired base digits
        size_t output_buffer_index = value->sign ? 1 : 0;

        for (int i = calc_buffer_start_nonzero; i >= 0; i--, output_buffer_index++) {
            // 0-th byte of calc_buffer is the least significant byte (small-endian)
            // 0-th byte of output buffer is the most significant digit (big-endian)

            // Start at the least significant byte of the value
            // (write it at the end of the output buffer).

            if (output_buffer_index >= buffer_length) {
                warn("output_buffer_index exceeds buffer length!\n");
                break;
            }

            // write the correct digit (char) from the alphabet to the current position
            uint8_t byte = get_byte_value_of_big_integer(calc_buffer, i);
            char c = alph[byte];
            buffer[output_buffer_index] = c;
        }

        // Add (negative) sign only if negative
        if (value->sign) {
            buffer[0] = '-';
        }

        // Null-byte terminating
        if (output_buffer_index >= buffer_length) {
            warn("Writing NULL-byte exceeds buffer length! buffer_length: %d, base: %d, val: \n", buffer_length, base);
            print_big_integer_hex(value);
            exit(5);
        }
        buffer[output_buffer_index] = 0x00;

        // Clear memory
        delete_big_integer(remaining_value);
        delete_big_integer(calc_buffer);

    } else {
        // Conversion to negative base:

        int base_abs = base < 0 ? -base : base;
        // Using adjusted (naive) algorithm for binary to negative base conversion from:
        // https://www.geeksforgeeks.org/convert-number-negative-base-representation/ case that
        // value is fully zero
        if (big_integer_is_zero(value, simd)) {
            buffer[0] = alph[0];
            buffer[1] = 0x00;
            return;
        }

        big_integer *temp = create_big_integer(value->length, false);
        big_integer *temp2 = create_big_integer(value->length, false);

        int index = -1;
        while (!big_integer_is_zero(value, simd)) {
            index++;
            // get modulo by: value % base and divide value by base: value /= base
            int remainder = (int) big_integer_division_int9_t(
                    value, base, temp, temp2, simd);  //=> value contains the division result

            if (remainder < 0) {
                remainder += base_abs;  // now remainder is modulo (always positive)
                big_integer_increment(value);
            }

            // remainder is char (starts with least significant digit) => write to reversed buffer,
            // reverse later
            if (remainder < 0 || remainder >= (int) strlen(alph)) {
                abort_err("[ERROR] Invalid remainder in convert binary to negative base.");
            }
            buffer[index] = alph[remainder];
        }

        // reverse buffer in-place
        int last_index = index;

        buffer[last_index + 1] = 0x00;  // NULL-byte string termination

        int j = 0;
        for (int i = index; i > j; i--, j++) {
            if (i >= (int) buffer_length || j >= (int) buffer_length) {
                abort_err("Invalid write index to output buffer (Index: %d or %d for size: %d)", i,
                          j, buffer_length);
            }
            char temp_start = buffer[j];
            buffer[j] = buffer[i];
            buffer[i] = temp_start;
        }

        delete_big_integer(temp);
        delete_big_integer(temp2);
    }
}
