#include "factorial.h"

uint64_t Factorial(int n)
{
    uint64_t r = 1;

    for (int i = 2; i <= n; ++i)
    {
        r *= i;
    }

    return r;
}