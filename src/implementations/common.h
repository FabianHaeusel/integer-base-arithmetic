#ifndef COMMON_H
#define COMMON_H

#include <limits.h>

/**
* @brief Populate the given lookup table
*
* This method stores the index of each char from the alphabet at the position indexed by that char.
* After calling this method alph[lut[(unsigned char) digit]] equals digit
*
* @param lut       The lookup table (an array char[UCHAR_MAX])
* @param alph_len  The length of the alphabet (abs(base))
* @param alph      The alphabet
*/
void generate_lut(unsigned char lut[UCHAR_MAX + 1], unsigned int alph_len, const char *alph);

#endif