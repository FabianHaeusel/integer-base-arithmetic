# integer-base-arithmetic
This repository contains a C-program for performing basic arithmetic operations on numbers in number systems with arbitrary integer bases.
It was created in the practical course "**Grundlagenpraktikum: Rechnerarchitektur (GRA)**" (computer architecture) in the summer semester 2023 by the students [@felixhauptmann](https://github.com/felixhauptmann), Dominik Schneider and [@FabianHaeusel](https://github.com/FabianHaeusel) at the Technical University Munich (TUM).

The german project title was "Arithmetik in Zahlensystemen mit ganzzahliger Basis".

Feel free to try out the program by building it using `make`, the help screen that provides information about the command-line-interface can be opened via the argument `-h`.
You can use this program to easily compute simple operations on absurdly large integers :)

## Features/options:
- specify the two operands (numbers to calculate with) as positional arguments after calling the executable
- specify the base of the number system in which you want to operate using `-b` followed by the base written in decimal notation
- specify the alphabet of the number system in which you want to operate using `-a` (mandatory, if `|base| > 10`). The length of the alphabet must be equal to `|base|` and the alphabet has to consist of printable ASCII characters
- the operator can be set using `-o` followed by either `+,-` or `*` for addition, subtraction and multiplication respectively (there is no division)
- tests that test the functionality and integrity of the program can be run using `-t`
- there are two different implementations of the same functionality: change between them using `-V <impl>`, you can choose between 0, 1 and 2
- you can benchmark the runtime of the program using `-B`
- list all implementations using `-l`

## Implementations
0. **Binary Conversion Implementation (SIMD)**: This implementation calculates the result of the arithmetic operation by first converting the numbers into binary, then performing the operation and then converting the result back to the original base. This implementation is enhanced by using SIMD (Single Instruction multiple data) operations (on a maximum of 128 bits).
1. **Binary Conversion Implementation (SISD)**: This implementation calculates the result of the arithmetic operation by first converting the numbers into binary, then performing the operation and then converting the result back to the original base. This implementation is not enhanced and therefore uses SISD (Single Instruction Single Data) operations.
2. **Naive Implementation**: This implementation calculates the result without conversion into another base (This is the fastest implementation).
