#include "binary_conversion_tests.h"

#include <stdlib.h>

#include "../../test.h"
#include "../../util.h"
#include "big_integer.h"
#include "big_integer_arithmetic.h"
#include "impl_binary_conversion.h"

/**
 * All functions run a couple of tests that test a single component function of the
 * binary_conversion "library".
 */

typedef struct Testcase_conversion {
    bool simd;
    size_t bytes_length;
    uint8_t (*bytes)[];
    bool sign;
    int16_t base;
    const char *alph;
    char *buffer;
    size_t buffer_length;
    char *expected;
} Testcase_conversion;

bool test_big_integer_conversion_to_any_base_executor(Testcase_conversion *t) {
    big_integer *value = create_big_integer_of_bytes(t->bytes_length, *t->bytes, t->sign);
    convert_big_integer_to_any_base(value, t->base, t->alph, t->buffer, t->buffer_length, t->simd);

    bool success = true;
    for (size_t j = 0; j < t->buffer_length; j++) {
        if (t->buffer[j] != t->expected[j]) {
            success = false;
            break;
        }
    }

    delete_big_integer(value);

    return success;
}

/**
 * Tests the conversion of binary numbers to numbers represented in any given arbitrary base from
 * -128 to 128.
 */
void test_big_integer_conversion_to_any_base(bool simd, Implementation impl) {
    TestResult tr = test_init_impl(impl, "big_integer conversion to any base");

    const char *alph =
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$&'()*+,-./"; //this alph supports max base 75

    Testcase_conversion test_cases[] = {

            // easy, base 10
            {simd, 1, &(uint8_t[]) {12},               false, 10, alph, malloc(3),  3, "12"},
            // negative base 10
            {simd, 1, &(uint8_t[]) {123},              true,  10, alph, malloc(5),  5, "-123"},
            // hex
            {simd, 2, &(uint8_t[]) {0xFE, 0xAF},       false, 16, alph, malloc(5),  5, "AFFE"},
            // base 2
            {simd, 3, &(uint8_t[]) {0x21, 0x43, 0x65}, false, 2,  alph, malloc(24), 24,
                                                                                       "11001010100001100100001"},
            // base (-2)
            {simd, 1, &(uint8_t[]) {15},               false, -2, alph, malloc(6),  6, "10011"},
            // negative in base (-2)
            {simd, 1, &(uint8_t[]) {3},                true,  -2, alph, malloc(5),  5, "1101"},
            // base (-3)
            {simd, 1, &(uint8_t[]) {12},               false, -3, alph, malloc(4),  4, "220"},
            // base 75 (62.942 => lowest to heighest: 17, 14, 11 => H, E, B => right oder: BEH
            {simd, 2, &(uint8_t[]) {0xDE, 0xF5},       false, 75, alph, malloc(4),  4, "BEH"},
    };

    int count = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < count; i++) {
        test_run(&test_cases[i], (bool (*)(void *)) test_big_integer_conversion_to_any_base_executor,
                 &tr, "TODO -> %s", "got %s", test_cases[i].expected, test_cases[i].buffer);
        free(test_cases[i].buffer);
    }

    test_finalize(tr);
}

typedef struct Testcase_addition {
    bool simd;

    size_t len_a;
    uint8_t (*a)[];
    bool sign_a;

    size_t len_b;
    uint8_t (*b)[];
    bool sign_b;

    char op;

    size_t len_exp;
    uint8_t (*exp)[];
    bool sign_exp;
} Testcase_addition;

bool test_binary_arithmetic_executor(Testcase_addition *t) {
    big_integer *a = create_big_integer_of_bytes(t->len_a, *t->a, t->sign_a);
    big_integer *b = create_big_integer_of_bytes(t->len_b, *t->b, t->sign_b);
    big_integer *expected = create_big_integer_of_bytes(t->len_exp, *t->exp, t->sign_exp);

    big_integer *mul_result = create_big_integer(t->len_exp, false);

    switch (t->op) {
        case '+':
            big_integer_addition(a, b, t->simd);
            break;
        case '-':
            big_integer_subtraction(a, b, t->simd);
            break;
        case '*':
            big_integer_multiplication(a, b, mul_result, t->simd);
            break;
        default:
            abort_err("No valid operation specified.\n");
    }

    bool success = true;
    for (size_t j = 0; j < t->len_exp; j++) {
        if ((t->op != '*' && get_byte_value_of_big_integer(expected, j) != 0 && j >= t->len_a) ||
            (t->op != '*' &&
             get_byte_value_of_big_integer(a, j) != get_byte_value_of_big_integer(expected, j)) ||
            (t->op == '*' && get_byte_value_of_big_integer(mul_result, j) !=
                             get_byte_value_of_big_integer(expected, j))) {
            success = false;
            break;
        }
    }

    delete_big_integer(a);
    delete_big_integer(b);
    delete_big_integer(expected);
    delete_big_integer(mul_result);

    return success;
}

/**
 * Tests the binary arithmetic functions (addition, subtraction, multiplication).
 * Note that the addition/subtraction methods do not check if the first big_integer is big enough
 * for the result of the addition.
 */
void test_binary_arithmetic(bool simd, Implementation impl) {
    TestResult tr = test_init_impl(impl, "arithmetic on big_integers");

    Testcase_addition test_cases[] = {

            // addition
            // 5+5 = 10
            {simd, 1, &(uint8_t[]) {5},                                  false, 1,  &(uint8_t[]) {
                    5},                                                                                  false, '+', 1, &(uint8_t[]) {
                    10},
                                                                                                                                       false},
            //-20 + 36 = 16
            {simd, 1, &(uint8_t[]) {20},                                 true,  1,  &(uint8_t[]) {
                    36},                                                                                 false, '+', 1, &(uint8_t[]) {
                    16},
                                                                                                                                       false},
            //-20 + (-55) = -75
            {simd, 1, &(uint8_t[]) {20},                                 true,  1,  &(uint8_t[]) {
                    55},                                                                                 true,  '+', 1, &(uint8_t[]) {
                    75},
                                                                                                                                       true},
            // 60 + (-14) = 46
            {simd, 1, &(uint8_t[]) {60},                                 false, 1,  &(uint8_t[]) {
                    14},                                                                                 true,  '+', 1, &(uint8_t[]) {
                    46},
                                                                                                                                       false},
            // 100 + 0 = 100
            {simd, 1, &(uint8_t[]) {100},                                false, 1,  &(uint8_t[]) {
                    0},                                                                                  true,  '+', 1, &(uint8_t[]) {
                    100},
                                                                                                                                       false},
            // big values: 885.080.511.659 + 3.585.614.078 = 888.666.125.737
            {simd, 5, &(uint8_t[]) {0xAB, 0xD4, 0xE8, 0x12, 0xCE},       false, 4,
                                                                                    &(uint8_t[]) {0xFE, 0x20, 0xB8,
                                                                                                  0xD5}, false, '+', 5,
                                                                                                                        &(uint8_t[]) {
                                                                                                                                0xA9,
                                                                                                                                0xF5,
                                                                                                                                0xA0,
                                                                                                                                0xE8,
                                                                                                                                0xCE}, false},

            // subtraction
            // 7 - 10 = -3
            {simd, 1, &(uint8_t[]) {7},                                  false, 1,  &(uint8_t[]) {
                    10},                                                                                 false, '-', 1, &(uint8_t[]) {
                    3},
                                                                                                                                       true},
            //-7 - 10 = -17
            {simd, 1, &(uint8_t[]) {7},                                  true,  1,  &(uint8_t[]) {
                    10},                                                                                 false, '-', 1, &(uint8_t[]) {
                    17},
                                                                                                                                       true},
            // 7 - (-10) = 17
            {simd, 1, &(uint8_t[]) {7},                                  false, 1,  &(uint8_t[]) {
                    10},                                                                                 true,  '-', 1, &(uint8_t[]) {
                    17},
                                                                                                                                       false},
            //-7 - (-10) = -7 + 10 = 3
            {simd, 1, &(uint8_t[]) {7},                                  true,  1,  &(uint8_t[]) {
                    10},                                                                                 true,  '-', 1, &(uint8_t[]) {
                    3},                                                                                                                false},
            // small - big: 123 - 58.975.131.579.787 = -58.975.131.579.664
            {simd, 6,
                      &(uint8_t[]) {
                              123,
                              0,
                              0,
                              0,
                              0,
                              0,
                              0,
                      },
                                                                         false, 6,  &(uint8_t[]) {0x8B, 0xB5, 0xC4,
                                                                                                  0x37, 0xA3,
                                                                                                  0x35}, false, '-', 6,
                                                                                                                        &(uint8_t[]) {
                                                                                                                                0x10,
                                                                                                                                0xB5,
                                                                                                                                0xC4,
                                                                                                                                0x37,
                                                                                                                                0xA3,
                                                                                                                                0x35}, true},
            // big - small
            {simd, 8, &(uint8_t[]) {0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF}, false, 1,  &(uint8_t[]) {0xFF},
                                                                                                         false, '-', 8, &(uint8_t[]) {
                    1, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF,
                    0xFF},                                                                                                             false},
            // SIMD numbers
            {simd, 16,
                      &(uint8_t[]) {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
                                    0X00, 0XFF, 0XFF},
                                                                         false, 15,
                                                                                    &(uint8_t[]) {0X01, 0X00, 0X00,
                                                                                                  0X00, 0X00, 0X00,
                                                                                                  0X00, 0X00, 0X00,
                                                                                                  0X00, 0X00, 0X00,
                                                                                                  0X00,
                                                                                                  0X00, 0X00},
                                                                                                         false, '-', 16,
                                                                                                                        &(uint8_t[]) {
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFF,
                                                                                                                                0XFE,
                                                                                                                                0XFF},
                                                                                                                                       false},
            // VERY BIG
            {simd, 35,
                      &(uint8_t[]) {0xFA, 0x68, 0x68, 0x87, 0x66, 0x87, 0x8E, 0x86, 0x79, 0x86, 0xDF, 0x76,
                                    0x89, 0x96, 0x87, 0xC6, 0xAB, 0x48, 0x23, 0x56, 0x84, 0x37, 0x52, 0x46,
                                    0x39, 0x78, 0x52, 0x46, 0x23, 0x58, 0x74, 0x23, 0x58, 0x74, 0x23},
                                                                         false, 31, &(uint8_t[]) {0x87, 0x66, 0x8A,
                                                                                                  0x87, 0x76, 0x86,
                                                                                                  0x6E, 0x65, 0x75,
                                                                                                  0x6E, 0x78,
                                                                                                  0xF6, 0x68, 0x68,
                                                                                                  0x6C, 0x68, 0x6B,
                                                                                                  0x86, 0x66, 0x6D,
                                                                                                  0x67, 0x67,
                                                                                                  0x76, 0xAC, 0x76,
                                                                                                  0x68, 0xDE, 0x67,
                                                                                                  0x98, 0x87, 0x0C},
                                                                                                         false, '-', 35,
                                                                                                                        &(uint8_t[]) {
                                                                                                                                0x73,
                                                                                                                                0x02,
                                                                                                                                0xDE,
                                                                                                                                0xFF,
                                                                                                                                0xEF,
                                                                                                                                0x00,
                                                                                                                                0x20,
                                                                                                                                0x21,
                                                                                                                                0x04,
                                                                                                                                0x18,
                                                                                                                                0x67,
                                                                                                                                0x80,
                                                                                                                                0x20,
                                                                                                                                0x2E,
                                                                                                                                0x1B,
                                                                                                                                0x5E,
                                                                                                                                0x40,
                                                                                                                                0xC2,
                                                                                                                                0xBC,
                                                                                                                                0xE8,
                                                                                                                                0x1C,
                                                                                                                                0xD0,
                                                                                                                                0xDB,
                                                                                                                                0x99,
                                                                                                                                0xC2,
                                                                                                                                0x0F,
                                                                                                                                0x74,
                                                                                                                                0xDE,
                                                                                                                                0x8A,
                                                                                                                                0xD0,
                                                                                                                                0x67,
                                                                                                                                0x23,
                                                                                                                                0x58,
                                                                                                                                0x74,
                                                                                                                                0x23},
                                                                                                                                       false},

            // multiplication
            // 25 * 0 = 0
            {simd, 1, &(uint8_t[]) {25},                                 false, 1,  &(uint8_t[]) {
                    0},                                                                                  false, '*', 1, &(uint8_t[]) {
                    0},
                                                                                                                                       false},
            // 69 * 1 = 69
            {simd, 1, &(uint8_t[]) {69},                                 false, 1,  &(uint8_t[]) {
                    1},                                                                                  false, '*', 1, &(uint8_t[]) {
                    69},
                                                                                                                                       false},
            // 42 * (-1) = -42
            {simd, 1, &(uint8_t[]) {42},                                 false, 1,  &(uint8_t[]) {
                    1},                                                                                  true,  '*', 1, &(uint8_t[]) {
                    42},
                                                                                                                                       true},
            // 11 * 11 = 121
            {simd, 1, &(uint8_t[]) {11},                                 false, 1,  &(uint8_t[]) {
                    11},                                                                                 false, '*', 1, &(uint8_t[]) {
                    121},
                                                                                                                                       false},
            // 5 * (-6 ) = -30
            {simd, 1, &(uint8_t[]) {5},                                  false, 1,  &(uint8_t[]) {
                    6},                                                                                  true,  '*', 1, &(uint8_t[]) {
                    30},                                                                                                               true},
            //(-7) * 11 = -77
            {simd, 1, &(uint8_t[]) {7},                                  true,  1,  &(uint8_t[]) {
                    11},                                                                                 false, '*', 1, &(uint8_t[]) {
                    77},
                                                                                                                                       true},
            //-14 * (-8) = 112
            {simd, 1, &(uint8_t[]) {14},                                 true,  1,  &(uint8_t[]) {
                    8},                                                                                  true,  '*', 1, &(uint8_t[]) {
                    112},
                                                                                                                                       false},
            // big numbers: 58.975.131.579.787 * 10.828.055 = 638.585.968.378.170.524.285
            {simd, 6, &(uint8_t[]) {0x8B, 0xB5, 0xC4, 0x37, 0xA3, 0x35}, false, 3,
                                                                                    &(uint8_t[]) {0x17, 0x39,
                                                                                                  0xA5}, false,
                                                                                                                '*',  // 22 9E 29 1A DD D1 AF 42 7D
                                                                                                                     9, &(uint8_t[]) {
                    0x7D, 0x42, 0xAF, 0xD1, 0xDD, 0x1A, 0x29, 0x9E,
                    0x22},                                                                                                             false},

    };

    int count = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < count; i++) {
        test_run(&test_cases[i], (bool (*)(void *)) test_binary_arithmetic_executor, &tr, "TODO", "TODO");
    }

    test_finalize(tr);
}

typedef struct Testcase_division {
    bool simd;

    size_t len_a;
    uint8_t (*a)[];
    bool sign_a;

    int16_t div;

    size_t len_exp;
    uint8_t (*exp)[];
    bool sign_exp;

    int16_t remainder_exp;  // euclidean remainder!

} Testcase_division;

bool test_big_integer_division_int9_executor(Testcase_division *t) {
    big_integer *a = create_big_integer_of_bytes(t->len_a, *t->a, t->sign_a);
    big_integer *expected = create_big_integer_of_bytes(t->len_exp, *t->exp, t->sign_exp);

    big_integer *temp1 = create_big_integer(t->len_a, false);
    big_integer *temp2 = create_big_integer(t->len_a, false);

    int16_t remainder = big_integer_division_int9_t(a, t->div, temp1, temp2, t->simd);

    bool success = remainder == t->remainder_exp;
    for (size_t j = 0; j < t->len_exp; j++) {
        if ((get_byte_value_of_big_integer(expected, j) != 0 && j >= t->len_a) ||
            get_byte_value_of_big_integer(a, j) != get_byte_value_of_big_integer(expected, j)) {
            success = false;
            break;
        }
    }

    delete_big_integer(a);
    delete_big_integer(expected);
    delete_big_integer(temp1);
    delete_big_integer(temp2);

    return success;
}

/**
 * Tests the division of a big_integer with a signed 9-bit (!) integer (only in valid range [-128;
 * 128)) that is passed as a 16-bit signed integer.
 */
void test_big_integer_division_int9(bool simd, Implementation impl) {
    TestResult tr = test_init_impl(impl, "big_integer division with 9bit signed integer");

    Testcase_division test_cases[] = {

            // multiplication
            // 16 / 4 = 4 R0
            {simd, 1, &(uint8_t[]) {16},  false, 4,  1, &(uint8_t[]) {4},  false, 0},
            // with remainder: 12 / 5 = 2 R2
            {simd, 1, &(uint8_t[]) {12},  false, 5,  1, &(uint8_t[]) {2},  false, 2},
            // negative: -20 / 4 = -5
            {simd, 1, &(uint8_t[]) {20},  true,  4,  1, &(uint8_t[]) {5},  true,  0},
            // negative remainder: -17 / 8 = -2 R-1
            {simd, 1, &(uint8_t[]) {17},  true,  8,  1, &(uint8_t[]) {2},  true,  -1},
            // negative remainder: -17 / -8 = 2 R-1
            {simd, 1, &(uint8_t[]) {17},  true,  -8, 1, &(uint8_t[]) {2},  false, -1},
            //-200 / -20 = 10
            {simd, 1, &(uint8_t[]) {200}, true,  20, 1, &(uint8_t[]) {10}, true,  0},

    };

    int count = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < count; i++) {
        test_run(&test_cases[i], (bool (*)(void *)) test_big_integer_division_int9_executor, &tr, "TODO",
                 "TODO");
    }

    test_finalize(tr);
}

typedef struct Testcase_shl {
    bool simd;

    size_t len_a;
    uint8_t (*a)[];
    bool sign_a;

    uint8_t shift_count;

    size_t len_exp;
    uint8_t (*exp)[];
    bool sign_exp;

} Testcase_shl;

bool test_big_integer_shl_executor(Testcase_shl *t) {
    big_integer *a = create_big_integer_of_bytes(t->len_a, *t->a, t->sign_a);
    big_integer *expected = create_big_integer_of_bytes(t->len_exp, *t->exp, t->sign_exp);

    big_integer_shl_bitwise_0_to_7(a, t->shift_count, t->simd);
    bool success = big_integer_is_equal(a, expected);

    delete_big_integer(a);
    delete_big_integer(expected);

    return success;
}

void test_big_integer_shl(bool simd, Implementation impl) {
    TestResult tr = test_init_impl(
            impl, "big_integer bitwise left-shift (shl) with shift values between 0 and 7");

    Testcase_shl test_cases[] = {

            // shl
            // 10110010 01001011 shl 3 => 00000101 10010010 01011000
            {simd, 3,  &(uint8_t[]) {0x4B, 0xB2, 0},       false, 3, 3, &(uint8_t[]) {0x58, 0x92, 0x05},       false},
            // 01100101 01000011 00100001 shl 7 => 00110010 10100001 10010000 10000000 (0x32 A1 90 80)
            {simd, 4,  &(uint8_t[]) {0x21, 0x43, 0x65, 0}, false, 7, 4,
                                                                        &(uint8_t[]) {0x80, 0x90, 0xA1, 0x32}, false},
            // negative number: -0x457 (-00000100 01010111) shl 2 => -0x115C (-00010001 01011100)
            {simd, 2,  &(uint8_t[]) {0x57, 0x04},          true,  2, 2, &(uint8_t[]) {0x5C, 0x11},             true},
            // very big number
            {simd, 21, &(uint8_t[]) {0X3E, 0X68, 0X7C, 0XFA, 0X7E, 0X82, 0X34, 0XE2, 0XB6, 0X3A, 0X28,
                                     0X49, 0X78, 0X59, 0X74, 0X9E, 0X49, 0X38, 0X88, 0X0F, 0},
                                                           false, 6, 21,
                                                                        &(uint8_t[]) {0X80, 0X0F, 0X1A, 0X9F, 0XBE,
                                                                                      0X9F, 0X20, 0X8D, 0XB8, 0XAD,
                                                                                      0X0E,
                                                                                      0X4A, 0X12, 0X5E, 0X16, 0X9D,
                                                                                      0X67, 0X12, 0X0E, 0XE2, 0X03},
                                                                                                               false},
            // shl 0 (edge-case)
            {simd, 3,  &(uint8_t[]) {0x65, 0x29, 0x23},    false, 0, 3, &(uint8_t[]) {0x65, 0x29, 0x23},
                                                                                                               false},

            // shifted bit(s) should be cut off
            // 00100011 00101001 01100101 shl 5 => [...] 01100101 00101100 10100000
            {simd, 3,  &(uint8_t[]) {0x65, 0x29, 0x23},    false, 5, 3, &(uint8_t[]) {0xA0, 0x2C, 0x65},
                                                                                                               false}

    };

    int count = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < count; i++) {
        test_run(&test_cases[i], (bool (*)(void *)) test_big_integer_shl_executor, &tr, "TODO", "TODO");
    }

    test_finalize(tr);
}

void binary_conversion_tests_sisd(Implementation impl) {
    test_big_integer_conversion_to_any_base(false, impl);
    test_binary_arithmetic(false, impl);
    test_big_integer_division_int9(false, impl);
    test_big_integer_shl(false, impl);
}

void binary_conversion_tests_simd(Implementation impl) {
    test_big_integer_conversion_to_any_base(true, impl);
    test_binary_arithmetic(true, impl);
    test_big_integer_division_int9(true, impl);
    test_big_integer_shl(true, impl);
}
