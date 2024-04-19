#include "impl_tests.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test.h"
#include "../util.h"

struct Env {
    implementation_t impl;
    char *res_buffer;
};

typedef struct Env Env;

struct Testcase {
    int base;
    const char *alph;
    const char *z1;
    const char *z2;
    char op;
    const char *result_expected;
};

typedef struct Testcase Testcase;

/**
 * @brief Calculates the number of chars necessary to represent the number x in the given base
 *
 * We're only using this method for impl_tests_test generation. log() is not actually part of our
 * implementation
 *
 * @param x     A number
 * @param base  Base of the target representation
 * @return      Number of chars needed to represent the number
 */
static size_t max_needed_chars_any_base(int x, int base) {
    size_t digits = (size_t) floor(log(abs(x)) / log(abs(base))) + 1;
    return base < 0 ? digits : digits + 1;  // add one for potential '-' char
}

static bool run_impl(Env *env, Testcase *t) {
    env->impl(t->base, t->alph, t->z1, t->z2, t->op, env->res_buffer);
    return strcmp(t->result_expected, env->res_buffer) == 0;
}

static void execute_test(Env *env, Testcase *t, TestResult *tr) {
    test_run_with_env(env, t, (void *) run_impl, tr, "%s %c %s = %s", "got %s", t->z1, t->op, t->z2,
                      t->result_expected, env->res_buffer);
}

/**
 * Converts a number to string with the given format specifier
 *
 * @param x     The number
 * @param fmt   The format specifier (%o|%d|%x)
 * @param buf   The buffer
 * @param len   The buffer size
 */
static void convert_base_8_10_16(int x, char *fmt, char *buf) {
    int neg = x < 0 ? true : false;

    if (neg) {
        *buf = '-';
        sprintf(buf + 1, fmt, -x);
    } else {
        sprintf(buf, fmt, x);
    }
}

/**
 * @brief Tests the specified operation with all combination of operands in the range [-limit, limit] in the given base with
 * the specified implementation
 *
 * This test uses the integer->string conversion provided by the standard library as a reference
 *
 * @param limit     Upper/Lower limit
 * @param base      The base (8|10|16)
 * @param op        Operation
 * @param impl      Implementation to be used
 */
static void base_x_pos_neg(int limit, int base, char op, Implementation impl) {
    TestResult tr = test_init_impl(impl, "positive/negative base%i values (%c)", base, op);

    char *fmt;
    char *alph;

    switch (base) {
        case 8:
            fmt = "%o";
            alph = "01234567";
            break;
        case 10:
            fmt = "%d";
            alph = "0123456789";
            break;
        case 16:
            fmt = "%x";
            alph = "0123456789abcdef";
            break;
        default:
            return;
    }

    size_t BUF_LEN = max_needed_chars_any_base(limit, base) + 1;  // number of chars plus NULL byte
    size_t RES_BUF_LEN = BUF_LEN * 2;  // result is at most twice as long as input

    char *z1_buf = malloc(BUF_LEN);
    check_alloc(z1_buf, BUF_LEN, "");

    char *z2_buf = malloc(BUF_LEN);
    check_alloc(z2_buf, BUF_LEN, "");

    char *expected_res_buf = malloc(RES_BUF_LEN);
    check_alloc(expected_res_buf, RES_BUF_LEN, "");

    char *res_buf = malloc(RES_BUF_LEN);
    check_alloc(res_buf, BUF_LEN, "");

    Env env = {impl.func, res_buf};

    for (int z1 = -limit; z1 <= limit; z1++) {
        for (int z2 = -limit; z2 <= limit; z2++) {

            convert_base_8_10_16(z1, fmt, z1_buf);
            convert_base_8_10_16(z2, fmt, z2_buf);

            int expected;

            switch (op) {
                case '+':
                    expected = z1 + z2;
                    break;
                case '-':
                    expected = z1 - z2;
                    break;
                case '*':
                    expected = z1 * z2;
                    break;
                default:
                    break;
            }

            convert_base_8_10_16(expected, fmt, expected_res_buf);

            Testcase t = {base, alph, z1_buf, z2_buf, op, expected_res_buf};
            execute_test(&env, &t, &tr);
        }
    }

    test_finalize(tr);

    free(z1_buf);
    free(z2_buf);
    free(expected_res_buf);
    free(res_buf);
}

static void base_neg2(Implementation impl) {
    TestResult tr = test_init_impl(impl, "base(-2) values");

    int base = -2;
    char *alph = "01";

    char *res_buf = malloc(32);  // we do not test for any results longer than 31 chars
    check_alloc(res_buf, 32, "");

    Env env = {impl.func, res_buf};

    Testcase testcases[] = {
            // +
            {base, alph, "0",  "0",  '+', "0"},
            {base, alph, "0",  "1",  '+', "1"},
            {base, alph, "0",  "10", '+', "10"},
            {base, alph, "0",  "11", '+', "11"},

            {base, alph, "1",  "0",  '+', "1"},
            {base, alph, "1",  "1",  '+', "110"},
            {base, alph, "1",  "10", '+', "11"},
            {base, alph, "1",  "11", '+', "0"},

            {base, alph, "10", "0",  '+', "10"},
            {base, alph, "10", "1",  '+', "11"},
            {base, alph, "10", "10", '+', "1100"},
            {base, alph, "10", "11", '+', "1101"},

            {base, alph, "11", "0",  '+', "11"},
            {base, alph, "11", "1",  '+', "0"},
            {base, alph, "11", "10", '+', "1101"},
            {base, alph, "11", "11", '+', "10"},

            // -
            {base, alph, "0",  "0",  '-', "0"},
            {base, alph, "0",  "1",  '-', "11"},
            {base, alph, "0",  "10", '-', "110"},
            {base, alph, "0",  "11", '-', "1"},

            {base, alph, "1",  "0",  '-', "1"},
            {base, alph, "1",  "1",  '-', "0"},
            {base, alph, "1",  "10", '-', "111"},
            {base, alph, "1",  "11", '-', "110"},

            {base, alph, "10", "0",  '-', "10"},
            {base, alph, "10", "1",  '-', "1101"},
            {base, alph, "10", "10", '-', "0"},
            {base, alph, "10", "11", '-', "11"},

            {base, alph, "11", "0",  '-', "11"},
            {base, alph, "11", "1",  '-', "10"},
            {base, alph, "11", "10", '-', "1"},
            {base, alph, "11", "11", '-', "0"},

            // *
            {base, alph, "0",  "0",  '*', "0"},
            {base, alph, "0",  "1",  '*', "0"},
            {base, alph, "0",  "10", '*', "0"},
            {base, alph, "0",  "11", '*', "0"},

            {base, alph, "1",  "0",  '*', "0"},
            {base, alph, "1",  "1",  '*', "1"},
            {base, alph, "1",  "10", '*', "10"},
            {base, alph, "1",  "11", '*', "11"},

            {base, alph, "10", "0",  '*', "0"},
            {base, alph, "10", "1",  '*', "10"},
            {base, alph, "10", "10", '*', "100"},
            {base, alph, "10", "11", '*', "110"},

            {base, alph, "11", "0",  '*', "0"},
            {base, alph, "11", "1",  '*', "11"},
            {base, alph, "11", "10", '*', "110"},
            {base, alph, "11", "11", '*', "1"},
    };

    int n = sizeof(testcases) / sizeof(Testcase);

    for (int i = 0; i < n; i++) {
        execute_test(&env, &testcases[i], &tr);
    }

    test_finalize(tr);

    free(res_buf);
}

static void other(Implementation impl) {
    TestResult tr = test_init_impl(impl, "other testcases");

    char *res_buf = malloc(4096);
    check_alloc(res_buf, 4096, "");

    Env env = {impl.func, res_buf};

    Testcase testcases[] = {
            {-10, "0123456789",
                         "23452348752893456792834657926230957238945728394578293457892374589237485",
                                                                "23845762734856723846572384576234785623489576", '*',
                                                                                                                     "30985840362188068317397079890340555446519773193503676564315398471455554745450214157617266"
                                                                                                                     "4273899261251167648056700"},
            {-10, "yh_4=xPg-I",
                         "_4=x_4=-gx_-I4=xPgI_-4=PxgI_P_4yIxg_4-I=xg_-4I=xg-_I4=xg-I_4g=x-I_4g=-x",
                                                                "_4-=xgP_g4=-xPg_4-=Pxg_4-=xgP_4=g-xP_4=-IxgP", '*',
                                                                                                                     "4yI-x-=y4P_h--yP-4hg4IgygI-Iy4=yxxx==PxhIgg4hI4xy4PgPxP=4hx4I-=gh=xxxx=g=x=xy_h=hxgPhg_"
                                                                                                                     "PP=_g4-II_Ph_xhhPgP=-yxPgyy"},
            {-3,  "EsK", "EEEsEsEKKKEKKKKKKEKEEEsKsEEsEEKssKK", "s",                                            '*',
                                                                                                                     "sEsEKKKEKKKKKKEKEEEsKsEEsEEKssKK"},
            {-3,  "EsK", "EEEsEsEKKKEKKKKKKEKEEEsKsEEsEEKssKK", "E",                                            '*', "E"}};

    int n = sizeof(testcases) / sizeof(Testcase);

    for (int i = 0; i < n; i++) {
        execute_test(&env, &testcases[i], &tr);
    }

    test_finalize(tr);

    free(res_buf);
}

static size_t generate_random_alph(char *alph) {
    char char_str[2] = {'\0', '\0'};

    alph[0] = '\0';

    size_t i = 0;
    while (i <= 1 || rand() > RAND_MAX / 10) {
        unsigned char symbol;
        do {
            symbol = rand() % (UCHAR_MAX + 1);
            char_str[0] = (char) symbol;
        } while ((char) symbol == '-' || (char) symbol == ' ' || !isprint((char) symbol) ||
                 strstr(alph, char_str) != NULL);

        alph[i] = (char) symbol;
        i++;
        alph[i] = '\0';
    }

    return i;
}

static void generate_random_num(const char *const alph, int base, size_t len, bool neg, char *z) {
    size_t base_abs = abs(base);

    if (neg && len > 1 && base > 0 && rand() < RAND_MAX / 2) {
        *z++ = '-';
        len--;
    }

    for (size_t i = 0; i < len; i++) {
        *z++ = alph[rand() % base_abs];
    }
    *z = '\0';
}

typedef struct {
    int base;
    const char *alph;
    const char *z1;
    const char *z2;
    char op;
    char *const *res_bufs;
    char *actual_str;
    size_t max_len;
} TestImplsCompareEnv;

static bool test_impls_compare_executor(TestImplsCompareEnv *testcase) {
    for (size_t i = 0; i < IMPLEMENTATIONS_COUNT; i++) {
        implementations[i].func(testcase->base, testcase->alph, testcase->z1, testcase->z2,
                                testcase->op, (testcase->res_bufs)[i]);
    }

    for (size_t i = 1; i < IMPLEMENTATIONS_COUNT; i++) {
        if (strncmp((testcase->res_bufs)[0], (testcase->res_bufs)[i], testcase->max_len) != 0) {
            char *actual = testcase->actual_str;
            *actual++ = '\n';
            for (size_t j = 0; j < IMPLEMENTATIONS_COUNT; j++) {
                actual += sprintf(actual, "    [%s]: \"%s\"\n", implementations[j].name,
                                  testcase->res_bufs[j]);
            }
            return false;
        }
    }

    *testcase->actual_str = '\0';
    return true;
}

static void test_impls_compare(size_t iterations, size_t max_len, size_t seed, char op) {
    TestResult tr = test_init("all", "comparing results with random inputs (%c)", op);

    size_t BUF_LEN = max_len + 1;
    size_t RES_BUF_LEN = BUF_LEN * 2;

    char *z1_buf = malloc(BUF_LEN);
    check_alloc(z1_buf, BUF_LEN, "");

    char *z2_buf = malloc(BUF_LEN);
    check_alloc(z2_buf, BUF_LEN, "");

    char *alph_buf = malloc(UCHAR_MAX + 2);
    check_alloc(alph_buf, UCHAR_MAX + 2, "");

    char *actual_str = malloc((RES_BUF_LEN + 100) * IMPLEMENTATIONS_COUNT);
    check_alloc(actual_str, (RES_BUF_LEN + 100) * IMPLEMENTATIONS_COUNT, "");

    char *res_bufs[IMPLEMENTATIONS_COUNT];

    for (size_t i = 0; i < IMPLEMENTATIONS_COUNT; i++) {
        res_bufs[i] = malloc(RES_BUF_LEN);
        check_alloc(res_bufs[i], RES_BUF_LEN, "");
    }

    TestImplsCompareEnv testcase = {0, alph_buf, z1_buf, z2_buf, op, res_bufs, actual_str, max_len};

    srand(seed);

    for (size_t i = 0; i < iterations; i++) {
        size_t base_abs = generate_random_alph(alph_buf);
        int base = rand() < RAND_MAX / 2 ? -((int) base_abs) : (int) base_abs;
        testcase.base = base;

        size_t len_1 = 1 + (rand() % max_len);  // [1; max_len]
        size_t len_2 = 1 + (rand() % max_len);  // [1; max_len]

        generate_random_num(alph_buf, base, len_1, true, z1_buf);
        generate_random_num(alph_buf, base, len_2, true, z2_buf);

        test_run(&testcase, (bool (*)(void *)) test_impls_compare_executor, &tr,
                 "\"%s\" %c \"%s\" with base %i and alphabet \"%s\"", "%s", z1_buf, op, z2_buf,
                 base, alph_buf, testcase.actual_str);
    }

    test_finalize(tr);

    free(z1_buf);
    free(z2_buf);
    free(alph_buf);
    free(actual_str);

    for (size_t i = 0; i < IMPLEMENTATIONS_COUNT; i++) {
        free(res_bufs[i]);
    }
}

void impl_tests_test(Implementation impl) {
    base_x_pos_neg(100, 8, '+', impl);
    base_x_pos_neg(100, 8, '-', impl);
    base_x_pos_neg(100, 8, '*', impl);

    base_x_pos_neg(100, 10, '+', impl);
    base_x_pos_neg(100, 10, '-', impl);
    base_x_pos_neg(100, 10, '*', impl);

    base_x_pos_neg(100, 16, '+', impl);
    base_x_pos_neg(100, 16, '-', impl);
    base_x_pos_neg(100, 16, '*', impl);

    base_neg2(impl);

    other(impl);
}

void impl_tests_test_all() {
    test_impls_compare(500, 50, 324235325, '+');
    test_impls_compare(500, 50, 324235325, '-');
    test_impls_compare(500, 50, 324235325, '*');
}
