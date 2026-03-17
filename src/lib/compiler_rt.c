#include <stdint.h>

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    uint64_t q = 0;
    while (n >= d) {
        n -= d;
        q++;
    }
    return q;
}

uint64_t __umoddi3(uint64_t n, uint64_t d) {
    while (n >= d) {
        n -= d;
    }
    return n;
}