#include "implementations.h"

#include "implementations/impl_binary_conversion/binary_conversion_tests.h"
#include "implementations/impl_binary_conversion/impl_binary_conversion.h"
#include "implementations/impl_naive/impl_naive.h"

const Implementation implementations[] = {
        { // default
                "Binary Conversion Implementation (SIMD)",
                "This implementation calculates the result of the arithmetic operation by first converting\n"
                "the numbers into binary, then performing the operation and then converting the result back\n"
                "to the original base. This implementation is enhanced by using SIMD (Single Instruction\n"
                "multiple data) operations (on a maximum of 128 bits).",
                arith_op_any_base__binary_conversion__simd, binary_conversion_tests_simd},
        {
                "Binary Conversion Implementation (SISD)",
                "This implementation calculates the result of the arithmetic operation by first converting\n"
                "the numbers into binary, then performing the operation and then converting the result back\n"
                "to the original base. This implementation is not enhanced and therefore uses SISD (Single\n"
                "Instruction Single Data) operations.",
                arith_op_any_base__binary_conversion__sisd, binary_conversion_tests_sisd
        },
        {
                "Naive Implementation",
                "This implementation calculates the result without conversion into another base.",
                impl_naive, impl_naive_test
        },

};

const size_t IMPLEMENTATIONS_COUNT = sizeof(implementations) / sizeof(Implementation);