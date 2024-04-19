#include <stdint.h>
#include <stdio.h>

#ifdef LOG
#include <stdarg.h>
#endif

/**
 * A method for debugging purposes. It behaves like printf, but puts a prefix "[DEBUG]" and
 * automatic new line "\\n".
 * @param arglist
 * @param ...
 */
void debug(__attribute__((unused)) const char *arglist, ...) {
#ifdef LOG
    va_list args;
    va_start(args, arglist);
    printf("[DEBUG] ");
    vfprintf(stdout, arglist, args);
    printf("\n");
    va_end(args);
#endif
}

/**
 * A method for debugging purposes. It behaves like printf, but puts a prefix "[WARN]" and
 * automatic new line "\\n".
 * @param arglist
 * @param ...
 */
void warn(__attribute__((unused)) const char *arglist, ...) {
#ifdef LOG
    va_list args;
    va_start(args, arglist);
    printf("[WARN] ");
    vfprintf(stdout, arglist, args);
    printf("\n");
    va_end(args);
#endif
}

/**
 * A method for debugging purposes. It behaves like printf, but puts a prefix "[ERROR]" and
 * automatic new line "\\n".
 * @param arglist
 * @param ...
 */
void error(__attribute__((unused)) const char *arglist, ...) {
#ifdef LOG
    va_list args;
    va_start(args, arglist);
    printf("[ERROR] ");
    vfprintf(stdout, arglist, args);
    printf("\n");
    va_end(args);
#endif
}

/**
 * Prints the byte in binary.
 * @param value
 */
void print_byte_binary(uint8_t value) {
    char buffer[8];
    for (int i = 0; i < 8; i++) {
        buffer[7 - i] = ((value >> i) & 0x01) ? '1' : '0';
    }
    printf("%s\n", buffer);
}