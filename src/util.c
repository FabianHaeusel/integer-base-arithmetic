#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief       Returns the number of chars of the longer string
 *
 * @param a     first NULL terminated string
 * @param b     second NULL terminated string
 * @return      strlen of the longest string
 */
size_t max_chars(const char *a, const char *b) {
    size_t n_a = strlen(a);
    size_t n_b = strlen(b);
    return n_a > n_b ? n_a : n_b;
}

size_t max_needed_chars_mul(const char *a, const char *b) { return max_chars(a, b) * 2 + 1; }

size_t max_needed_chars_add_sub(const char *a, const char *b) { return max_chars(a, b) + 2; }

_Noreturn void abort_err(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Aborting: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static _Noreturn void abort_alloc(size_t bytes, char *desc) {
    abort_err("Could not allocate memory for %s! (%zu bytes)", desc, bytes);
}

void check_alloc(void *p, size_t bytes, char *desc) {
    if (p == NULL) {
        abort_alloc(bytes, desc);
    }
}
