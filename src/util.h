#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/**
 * Takes two numbers of any (shared) base which may be prefixed by a sign and returns a number of
 * chars that is guaranteed to be big enough to accommodate the result of adding/subtracting a and b
 *
 * @param a     First operand
 * @param b     Second operand
 * @return
 */
size_t max_needed_chars_add_sub(const char *a, const char *b);

/**
 * Takes two numbers of any (shared) base which may be prefixed by a sign and returns a number of
 * chars that is guaranteed to be big enough to accommodate the result of multiplying a and b
 *
 * @param a     First operand
 * @param b     Second operand
 * @return
 */
size_t max_needed_chars_mul(const char *a, const char *b);

/**
 * @brief Check if allocation was successful, otherwise abort
 *
 * @param p
 * @param bytes
 * @param desc
 */
void check_alloc(void *p, size_t bytes, char *desc);

/**
 * @brief Abort execution
 *
 * This method should be called if anything unexpected happens.
 *
 * @param fmt
 * @param ...
 */
_Noreturn void abort_err(const char *fmt, ...);

#endif