#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bench.h"
#include "implementations.h"
#include "test.h"
#include "util.h"

static char *default_alph = NULL;  // Default (0..9) gets generated if necessary
static char *result = NULL;

// Defining the long_option help
static struct option long_options[] = {{"help", no_argument, NULL, 'h'},
                                       {NULL, 0,             NULL, 0}};

const char *about_msg = "This program calculates the sum/difference/product of two numbers.\n";

/// Format string expects char*, size_t, 8*(char*) (progname, IMPLEMENTATIONS_COUNT - 1, 8*progname)
static const char *usage_msg =
        "Usage:\n"
        "  %s [-o (+|-|*)] [-b <base>] [-a <alphabet>] [-V (0-%zu)] [-B[<repetitions>]] z1 z2\n"
        "  %s -t [-V <impl>]\n"
        "  %s -l\n"
        "  %s -h | --help\n"
        "\n"
        "Examples:\n"
        "  %s 100 50\n"
        "  %s -V 1 -o '*' -b 5 24 10\n"
        "  %s -a abcdefg -b 7 -o - -- -abc dfg\n"
        "  %s -B10 100 50\n"
        "  %s -V 0 -t\n";

/// Format string expects size_t char* (IMPLEMENTATIONS_COUNT - 1, progname)
static const char *help_msg =
        "Arguments:\n"
        "  z1                  Operand 1 (augend/subtrahend/multiplicand).\n"
        "  z2                  Operand 2 (addend/minuend/multiplier).\n"
        "                      The operands must not contain any characters not contained in the "
        "alphabet.\n"
        "                      If there are negative operands, separate them from all other arguments "
        "by --.\n"
        "\n"
        "Options:\n"
        "  -h --help           Show this help message and exit.\n"
        "  -t                  Run tests.\n"
        "                      If no implementation is specified, all implementations will be tested.\n"
        "  -b <base>           The base (|base| > 1). [default: 10]\n"
        "  -o (+|-|*)          The operator. [default: +]\n"
        "  -a <alphabet>       The alphabet. (mandatory if |base| > 10) [default: \"0123456789\"]\n"
        "                      The length of the alphabet must be equal to |base|.\n"
        "                      The alphabet has to consist of printable ASCII characters\n"
        "  -V (0-%zu)            Implementation. [default: 0]\n"
        // documented behavior of getopt: If the option has an optional argument, it must be written
        // directly after the option character if present.
        "  -B[<repetitions>]   Measure runtime.\n"
        "                      Repeat the calculation as often as specified. [default: 3]\n"
        "  -l                  List all implementations and exit.\n";

static void print_usage(const char *progname, FILE *stream) {
    fprintf(stream, usage_msg, progname, IMPLEMENTATIONS_COUNT - 1, progname, progname, progname,
            progname, progname, progname, progname, progname);
}

/**
 * @brief Print help message
 *
 * @param progname  The name of the program (argv[0])
 */
static void print_help(const char *progname) {
    printf("%s", about_msg);
    printf("\n");
    print_usage(progname, stdout);
    printf("\n");
    printf(help_msg, IMPLEMENTATIONS_COUNT - 1);
}

/**
 * @brief List all available implementations
 */
static void list_impls() {
    printf("Available Implementations:");
    for (size_t i = 0; i < IMPLEMENTATIONS_COUNT; i++) {
        if (i == 0) {
            printf("\n[%zu] (default)\n%s: %s\n", i, implementations[i].name,
                   implementations[i].description);
        } else {
            printf("\n[%zu]\n%s: %s\n", i, implementations[i].name,
                   implementations[i].description);
        }
    }
}

static void cleanup() {
    free(default_alph);
    free(result);
}

/**
 * @brief Exit with the provided error message
 *
 * This method prints the usage message and the provided error message, then exits with exit code
 * EXIT_FAILURE.
 *
 * @param progname  The name of the program (argv[0])
 * @param fmt       The format string
 * @param ...       The arguments for the format string
 */
static _Noreturn void exit_err_msg(const char *progname, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    print_usage(progname, stdout);
    cleanup();
    exit(EXIT_FAILURE);
}

/**
 * @brief Create the default alphabet for |bases| <= 10
 *
 * @param alph      A pointer to a buffer that is at least base_abs + 1 wide
 * @param base_abs  Base of the numeral system
 */
static void create_alph(char *alph, size_t base_abs) {
    char c = '0';
    char *alph_end = alph + base_abs;

    while (alph < alph_end) {
        *alph++ = c++;
    }

    *alph = '\0';
}

/**
 * @brief Check if the passed alphabet is valid.
 *
 * @param alph     Alphabet of the numbers
 * @param base     Base of the numeral system
 */
static void check_alphabet(const char *progname, char *alph, int base) {
    size_t length = strlen(alph);

    // Check for duplicates in the alphabet
    for (size_t i = 0; i < length; i++) {
        if (base > 0 && alph[i] == '-') {
            exit_err_msg(progname,
                         "The alphabet contains the symbol '-' which is invalid when using a "
                         "positive base.\n");
        }
        if (!isprint(alph[i])) {
            exit_err_msg(progname, "The alphabet contains values which are not printable ASCII characters.\n");
        }
        for (size_t j = i + 1; j < length; j++) {
            if (alph[i] == alph[j]) {
                exit_err_msg(progname,
                             "The alphabet \"%s\" contains the symbol '%c' more than once.\n", alph,
                             alph[i]);
            }
        }
    }

    // Check if the alphabet contains the right amount of elements
    if (length != (size_t) abs(base)) {
        exit_err_msg(progname, "The size of the alphabet \"%s\" does not match with the base %d.\n",
                     alph, base);
    }
}

/**
 * @brief Check if the given numbers are valid.
 *
 * @param progname  The name of the program (argv[0)
 * @param z1        First number
 * @param z1        Second number
 * @param alph      The alphabet of the numbers
 * @param base      The base
 */
static void check_numbers(const char *progname, const char *z1, const char *z2, const char *alph,
                          const int base) {
    // Check if the number is negative -> Base has to be positive then
    if (base > 0 && *z1 == '-') {
        z1++;
    }
    if (base > 0 && *z2 == '-') {
        z2++;
    }

    size_t len1 = strlen(z1);
    size_t len2 = strlen(z2);

    // Check if the passed numbers are empty
    if (len1 < 1) {
        exit_err_msg(progname, "The first number has an invalid size of %ld.\n", strlen(z1));

    } else if (len2 < 1) {
        exit_err_msg(progname, "The second number has an invalid size of %ld.\n", strlen(z2));
    }

    // Check if every character of the numbers is in the alphabet
    for (size_t i = 0; i < len1; i++) {
        if (strchr(alph, z1[i]) == NULL) {
            exit_err_msg(progname,
                         "The first number contains characters that are not in the alphabet: "
                         "\"%s\" does not contain '%c'.\n",
                         alph, z1[i]);
        }
    }

    for (size_t i = 0; i < len2; i++) {
        if (strchr(alph, z2[i]) == NULL) {
            exit_err_msg(progname,
                         "The second number contains characters that are not in the alphabet: "
                         "\"%s\" does not contain '%c'.\n",
                         alph, z2[i]);
        }
    }
}

int main(int argc, char **argv) {

    const char *progname = argv[0];

    bool test = false;
    bool benchmark = false;
    size_t benchmark_repetitions = 3;  // Default, 3 repetitions for the benchmark tests

    char operator = '+';  // Default operator: +
    char *alph = NULL;   // Default (0..9) gets generated if necessary
    int base = 10;       // Default base: 10

    bool implementation_specified =
            false;  // This flag is used for testing because if no impl is specified we test all impls
    size_t implementation = 0;  // Default use the main implementation

    int opt;
    while ((opt = getopt_long(argc, argv, ":V:B::b:a:o:tlh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'V':
                implementation_specified = true;
                implementation = strtoull(optarg, NULL, 10);
                break;
            case 'B':
                benchmark = true;
                if (optarg != NULL) {
                    benchmark_repetitions = strtoull(optarg, NULL, 10);
                }
                break;
            case 'b':
                base = (int) strtol(optarg, NULL, 10);
                break;
            case 'a':
                alph = optarg;
                break;
            case 'o':
                operator = *optarg;
                break;
            case 't':
                test = 1;
                break;
            case 'h':
                print_help(progname);
                return EXIT_SUCCESS;
            case 'l':
                list_impls();
                return EXIT_SUCCESS;
            case '?':
                // If character not recognized, opt = '?' and option character stored in optopt
                if (isprint(optopt)) {
                    exit_err_msg(progname, "Unknown option -%c.\n", optopt);
                } else {
                    exit_err_msg(progname, "Unknown option character \\x%x.\n", optopt);
                }
            default:
                exit_err_msg(progname, "Option -%c requires an argument.\n", optopt);
        }
    }

    // Check if the implementation is valid
    if (implementation >= IMPLEMENTATIONS_COUNT) {
        exit_err_msg(progname, "Invalid implementation.\n");
    }

    // Run the tests if the test flag is set
    if (test && implementation_specified) {
        test_impl(implementations[implementation]);
        return EXIT_SUCCESS;
    } else if (test) {
        test_all_impls();
        return EXIT_SUCCESS;
    }

    // Check if the operator is valid
    if (!(operator == '+' || operator == '*' || operator == '-')) {
        exit_err_msg(progname, "Invalid operator: '%c'\n", operator);
    }

    // Check if the base is valid
    if (base <= 1 && base >= -1) {
        exit_err_msg(progname, "Invalid base: %d\n", base);
    }

    // Check if the alphabet is valid
    if (alph == NULL) {
        if (base >= -10 && base <= 10) {
            // use default alphabet (0..9)
            size_t base_abs = abs(base);
            default_alph = malloc(base_abs + 1);
            if (default_alph == NULL) {
                fprintf(stderr, "Could not allocate memory for the alphabet buffer.\n");
                return EXIT_FAILURE;
            }
            create_alph(default_alph, base_abs);
            alph = default_alph;
        } else {
            exit_err_msg(progname, "No alphabet specified. (-a <alphabet>)\n");
        }
    } else {
        check_alphabet(progname, alph, base);
    }

    // Read the positional arguments (z1, z2)
    char *z1;
    char *z2;

    if (optind == argc - 2) {
        z1 = argv[optind];
        z2 = argv[optind + 1];
    } else {
        // incorrect number of positional arguments were passed
        exit_err_msg(progname, "The program expects 2 operands but %i arguments were passed.\n",
                     (argc - optind));
    }

    // Check if the numbers are valid
    check_numbers(progname, z1, z2, alph, base);

    // Initialize result buffer
    size_t buffer_size;
    if (operator == '+' || operator == '-') {
        buffer_size = max_needed_chars_add_sub(z1, z2);
    } else {
        buffer_size = max_needed_chars_mul(z1, z2);
    }

    result = malloc(buffer_size + 1);

    if (result == NULL) {
        fprintf(stderr, "Could not allocate memory for the result buffer.\n");
        cleanup();
        return EXIT_FAILURE;
    }

    // Run the benchmarks if runtime flag is set
    if (benchmark) {
        long double time = bench(implementations[implementation], benchmark_repetitions, base, alph,
                                 z1, z2, operator, result);
        fprintf(stdout, "[%s] took %Lf ms to execute the calculation %zu times.\n",
                implementations[implementation].name, time * 1000, benchmark_repetitions);
        fprintf(stdout, "[%s] mean execution time: %Lf ms.\n", implementations[implementation].name,
                (time / benchmark_repetitions) * 1000);
    } else {
        // Call the selected implementation
        implementations[implementation].func(base, alph, z1, z2, operator, result);
    }

    fprintf(stdout, "%s %c %s = %s\n", z1, operator, z2, result);

    cleanup();
    return EXIT_SUCCESS;
}