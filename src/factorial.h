#pragma once

#include <cstdint>

inline constexpr uint64_t FACT[] =
{
    1ULL,
    1ULL,
    2ULL,
    6ULL,
    24ULL,
    120ULL,
    720ULL,
    5040ULL,
    40320ULL,
    362880ULL,
    3628800ULL,
    39916800ULL,
    479001600ULL,
    6227020800ULL,
    87178291200ULL,
    1307674368000ULL,
    20922789888000ULL,
    355687428096000ULL,
    6402373705728000ULL,
    121645100408832000ULL,
    2432902008176640000ULL
};

inline uint64_t Factorial(int n)
{
    return FACT[n];
}