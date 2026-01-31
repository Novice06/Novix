/*
    !! WARNING !!
    This AI slop
*/

#include "feistel64.h"

/* rotation helpers (pas toujours dispo en kernel) */
static inline uint32_t rotl32(uint32_t x, int r)
{
    return (x << r) | (x >> (32 - r));
}

/*
 * Fonction F :
 * - rapide
 * - non linéaire
 * - suffisante pour permutation d’IDs
 */
static inline uint32_t F(uint32_t x, uint64_t key, int round)
{
    x ^= (uint32_t)(key >> (round * 8));
    x *= 0x9E3779B9u;            // golden ratio
    x = rotl32(x, 11);
    x ^= x >> 7;
    return x;
}

/*
 * Feistel 64 bits (bijectif)
 */
uint64_t feistel64(uint64_t v, uint64_t key)
{
    uint32_t L = (uint32_t)(v >> 32);
    uint32_t R = (uint32_t)(v & 0xFFFFFFFFu);

    for (int i = 0; i < 8; i++) {
        uint32_t tmp = L ^ F(R, key, i);
        L = R;
        R = tmp;
    }

    return ((uint64_t)L << 32) | R;
}

/*
 * Inverse exact (optionnel mais propre)
 */
uint64_t feistel64_inv(uint64_t v, uint64_t key)
{
    uint32_t L = (uint32_t)(v >> 32);
    uint32_t R = (uint32_t)(v & 0xFFFFFFFFu);

    for (int i = 7; i >= 0; i--) {
        uint32_t tmp = R;
        R = L ^ F(R, key, i);
        L = tmp;
    }

    return ((uint64_t)L << 32) | R;
}
