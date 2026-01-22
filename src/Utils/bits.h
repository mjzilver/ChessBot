#pragma once
#include <cassert>
#include <cstdint>

// count trailing zeros
constexpr unsigned int ctz(uint64_t x) {
    assert(x != 0 && "ctz called with zero");

    unsigned int n = 0;
    while ((x & 1) == 0) {
        x >>= 1;
        n++;
    }
    return n;
}

// Most significant bit
constexpr unsigned int msb(uint64_t x) {
    assert(x != 0 && "msb called with zero");

    unsigned int n = 63;
    while ((x & (1ULL << n)) == 0) {
        n--;
    }
    return n;
}