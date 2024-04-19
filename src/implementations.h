#ifndef IMPLEMENTATIONS_H
#define IMPLEMENTATIONS_H

#include <stdlib.h>

/**
 * The signature of our implementation as specified in the task
 */
#define IMPL_SIGNATURE \
    (int base, const char *alph, const char *z1, const char *z2, char op, char *result)

/**
 * @param base      The base.           base > 1 || base < -1
 * @param alph      The alphabet.       Must not contain '-' if base > 0 or any duplicate values.
 *                                      len(alph) == abs(base)
 * @param z1        The first operand.  May start with '-' if base is positive. Any following
 *                                      characters must be contained in alph.
 * @param z2        The second operand. Same rules apply as for z1.
 * @param op        The operator.       Must be any of ['+', '-', '*'].
 * @param result    The result buffer.  The result will be stored as a NULL terminated string. Must
 *                                      be large enough to be able to accommodate the result.
 */
typedef void(*implementation_t)IMPL_SIGNATURE;

typedef struct Implementation Implementation;

typedef void (*test_t)(Implementation);

struct Implementation {
    const char *name;
    const char *description;
    implementation_t func;
    test_t test;
};

extern const Implementation implementations[];
extern const size_t IMPLEMENTATIONS_COUNT;

#endif
