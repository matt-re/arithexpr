Provides a C++ function to evaluate a simple arithmetric expression.
Supports addition, subtraction, multiplication and division of integer numbers only and returns an integer result.
Expressions are parsed left to right and all operators have the same precedence, also known as chain calculation mode.

## Build
On Linux and macOS use:

```
make
./calc 1 + 2 \* 3
./calc "1 + 2 * 3"
```

Tested with GCC 16 on Linux and Clang 17 on macOS.

## Test
```
make test
```

