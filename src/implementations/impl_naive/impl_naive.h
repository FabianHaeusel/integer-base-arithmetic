#ifndef IMPL_NAIVE_H
#define IMPL_NAIVE_H

#include "../../implementations.h"

/**
 * \brief Naive implementation
 *
 * This implementation calculates the result without conversion into another base.
 * This is being achieved by performing the following calculation
 * for each pair of digits from least significant to most significant with an initial carry value of
 * 0: <li> looking up the indices in the alphabet <li> applying the operator to the indices <li>
 * adding the carry value <li> adding/subtracting abs(base) from the result depending on
 * over/underflow <li> setting the carry value to -1/1 depending on over/underflow <li> negating the
 * carry value if the base is negative
 *
 * @param base      The base.           base > 1 || base < -1
 * @param alph      The alphabet.       Must not contain '-' if base > 0 or any duplicate values.
 * len(alph) == abs(base)
 * @param z1        The first operand.  May start with '-' if base is positive. Any following
 * characters must be contained in alph.
 * @param z2        The second operand. Same rules apply as for z1.
 * @param op        The operator.       Must be any of ['+', '-', '*'].
 * @param result    The result buffer.  The result will be stored as a NULL terminated string. Must
 * be large enough to be able to accommodate the result.
 */
void impl_naive IMPL_SIGNATURE;

void impl_naive_test(Implementation impl);

#endif