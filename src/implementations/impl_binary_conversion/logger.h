#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

void debug(const char *arglist, ...);

void warn(const char *arglist, ...);

void error(__attribute__((unused)) const char *arglist, ...);

void print_byte_binary(uint8_t value);

#endif
