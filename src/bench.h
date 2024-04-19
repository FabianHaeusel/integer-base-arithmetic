#ifndef BENCH_H
#define BENCH_H

#include "implementations.h"

/**
 * @brief Benchmarks the given implementation by calculating as often as specified
 *
 * @param impl          The implementation
 * @param iterations    The number of iterations
 * @param base          The base.           base > 1 || base < -1
 * @param alph          The alphabet.       Must not contain '-' if base > 0 or any duplicate
 *                                          values. len(alph) == abs(base)
 * @param z1            The first operand.  May start with '-' if base is positive. Any following
 *                                          characters must be contained in alph.
 * @param z2            The second operand. Same rules apply as for z1.
 * @param op            The operator.       Must be any of ['+', '-', '*'].
 * @param result        The result buffer.  The result will be stored as a NULL terminated string.
 *                                          Must be large enough to be able to accommodate the
 *                                          result.
 * @return              The mean time in seconds
 */
long double bench(Implementation impl, size_t iterations, int base, const char *alph,
                  const char *z1, const char *z2, char op, char *result);

#endif
