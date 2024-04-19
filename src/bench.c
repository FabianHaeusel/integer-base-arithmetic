#include "bench.h"

#include <time.h>

static long double current_time() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9L;
}

long double bench(Implementation impl, size_t iterations, int base, const char *alph,
                  const char *z1, const char *z2, char op, char *result) {
    implementation_t func = impl.func;

    long double t = current_time();

    for (size_t i = 0; i < iterations; i++) {
        func(base, alph, z1, z2, op, result);
    }

    return (current_time() - t);
}
