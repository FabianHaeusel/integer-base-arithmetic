#include "common.h"

void generate_lut(unsigned char lut[UCHAR_MAX + 1], unsigned int alph_len, const char *alph) {
    for (unsigned int i = 0; i < alph_len; i++) {
        lut[(unsigned char) alph[i]] = i;
    }
}