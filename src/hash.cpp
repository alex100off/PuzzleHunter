#include "hash.h"
#include <cstring>

// ============================================================================
// SHA-256
// ВАЖНО: длина сообщения в padding записывается в BIG-ENDIAN
// ============================================================================
namespace {

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t bsig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t bsig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t ssig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint32_t ssig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

} // anonymous namespace

void sha256(const uint8_t* data, size_t len, uint8_t out[32])
{
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Padding: сообщение + 0x80 + нули + длина в big-endian (8 байт)
    size_t new_len = ((len + 8) / 64 + 1) * 64;
    uint8_t* block = new uint8_t[new_len]();
    std::memcpy(block, data, len);
    block[len] = 0x80;

    // Длина в БИТАХ, записывается в BIG-ENDIAN (SHA-256 требует именно так)
    uint64_t bits = (uint64_t)len * 8;
    block[new_len - 8] = (uint8_t)(bits >> 56);
    block[new_len - 7] = (uint8_t)(bits >> 48);
    block[new_len - 6] = (uint8_t)(bits >> 40);
    block[new_len - 5] = (uint8_t)(bits >> 32);
    block[new_len - 4] = (uint8_t)(bits >> 24);
    block[new_len - 3] = (uint8_t)(bits >> 16);
    block[new_len - 2] = (uint8_t)(bits >> 8);
    block[new_len - 1] = (uint8_t)(bits);

    // Обработка каждого 64-байтного блока
    for (size_t off = 0; off < new_len; off += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = ((uint32_t)block[off + i*4] << 24) |
                   ((uint32_t)block[off + i*4 + 1] << 16) |
                   ((uint32_t)block[off + i*4 + 2] << 8) |
                   ((uint32_t)block[off + i*4 + 3]);
        for (int i = 16; i < 64; ++i)
            w[i] = ssig1(w[i-2]) + w[i-7] + ssig0(w[i-15]) + w[i-16];

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], t = h[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = t + bsig1(e) + ch(e, f, g) + K256[i] + w[i];
            uint32_t t2 = bsig0(a) + maj(a, b, c);
            t = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += t;
    }

    delete[] block;

    // Вывод в big-endian
    for (int i = 0; i < 8; ++i) {
        out[i*4 + 0] = (uint8_t)(h[i] >> 24);
        out[i*4 + 1] = (uint8_t)(h[i] >> 16);
        out[i*4 + 2] = (uint8_t)(h[i] >> 8);
        out[i*4 + 3] = (uint8_t)(h[i]);
    }
}

// ============================================================================
// RIPEMD-160
// ВАЖНО: длина сообщения в padding записывается в LITTLE-ENDIAN
// Реализация основана на оригинальной спецификации RIPEMD-160
// ============================================================================
namespace {

inline uint32_t rotl160(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

// Булевы функции RIPEMD-160 (5 штук, используются в левой и правой ветвях)
inline uint32_t F0(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
inline uint32_t F1(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
inline uint32_t F2(uint32_t x, uint32_t y, uint32_t z) { return (x | ~y) ^ z; }
inline uint32_t F3(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
inline uint32_t F4(uint32_t x, uint32_t y, uint32_t z) { return x ^ (y | ~z); }

// Макрос одного раунда RIPEMD-160
// a,b,c,d,e — рабочие переменные
// F — булева функция
// X — массив 16 слов блока
// K — константа раунда
// r — индекс слова
// s — сдвиг
#define RIPEMD_ROUND(a, b, c, d, e, F, X, K, r, s) \
    do { \
        (a) += F((b), (c), (d)) + (X)[(r)] + (K); \
        (a) = rotl160((a), (s)) + (e); \
        (c) = rotl160((c), 10); \
    } while (0)

} // anonymous namespace

void ripemd160(const uint8_t* data, size_t len, uint8_t out[20])
{
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xefcdab89;
    uint32_t h2 = 0x98badcfe;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xc3d2e1f0;

    // Padding: сообщение + 0x80 + нули + длина в little-endian (8 байт)
    size_t new_len = ((len + 8) / 64 + 1) * 64;
    uint8_t* block = new uint8_t[new_len]();
    std::memcpy(block, data, len);
    block[len] = 0x80;

    // Длина в БИТАХ, записывается в LITTLE-ENDIAN (RIPEMD-160 требует именно так)
    uint64_t bits = (uint64_t)len * 8;
    block[new_len - 8] = (uint8_t)(bits);
    block[new_len - 7] = (uint8_t)(bits >> 8);
    block[new_len - 6] = (uint8_t)(bits >> 16);
    block[new_len - 5] = (uint8_t)(bits >> 24);
    block[new_len - 4] = (uint8_t)(bits >> 32);
    block[new_len - 3] = (uint8_t)(bits >> 40);
    block[new_len - 2] = (uint8_t)(bits >> 48);
    block[new_len - 1] = (uint8_t)(bits >> 56);

    // Обработка каждого 64-байтного блока
    for (size_t off = 0; off < new_len; off += 64) {
        uint32_t X[16];
        // Чтение слов в little-endian (RIPEMD-160 использует LE)
        for (int i = 0; i < 16; ++i)
            X[i] = ((uint32_t)block[off + i*4]) |
                   ((uint32_t)block[off + i*4 + 1] << 8) |
                   ((uint32_t)block[off + i*4 + 2] << 16) |
                   ((uint32_t)block[off + i*4 + 3] << 24);

        // Рабочие переменные для левой и правой ветвей
        uint32_t al = h0, bl = h1, cl = h2, dl = h3, el = h4;
        uint32_t ar = h0, br = h1, cr = h2, dr = h3, er = h4;

        // ============================================================
        // Раунды 0-15: левая F0 (K=0), правая F4 (K=0x50a28be6)
        // ============================================================
        RIPEMD_ROUND(al, bl, cl, dl, el, F0, X, 0x00000000,  0, 11);
        RIPEMD_ROUND(el, al, bl, cl, dl, F0, X, 0x00000000,  1, 14);
        RIPEMD_ROUND(dl, el, al, bl, cl, F0, X, 0x00000000,  2, 15);
        RIPEMD_ROUND(cl, dl, el, al, bl, F0, X, 0x00000000,  3, 12);
        RIPEMD_ROUND(bl, cl, dl, el, al, F0, X, 0x00000000,  4,  5);
        RIPEMD_ROUND(al, bl, cl, dl, el, F0, X, 0x00000000,  5,  8);
        RIPEMD_ROUND(el, al, bl, cl, dl, F0, X, 0x00000000,  6,  7);
        RIPEMD_ROUND(dl, el, al, bl, cl, F0, X, 0x00000000,  7,  9);
        RIPEMD_ROUND(cl, dl, el, al, bl, F0, X, 0x00000000,  8, 11);
        RIPEMD_ROUND(bl, cl, dl, el, al, F0, X, 0x00000000,  9, 13);
        RIPEMD_ROUND(al, bl, cl, dl, el, F0, X, 0x00000000, 10, 14);
        RIPEMD_ROUND(el, al, bl, cl, dl, F0, X, 0x00000000, 11, 15);
        RIPEMD_ROUND(dl, el, al, bl, cl, F0, X, 0x00000000, 12,  6);
        RIPEMD_ROUND(cl, dl, el, al, bl, F0, X, 0x00000000, 13,  7);
        RIPEMD_ROUND(bl, cl, dl, el, al, F0, X, 0x00000000, 14,  9);
        RIPEMD_ROUND(al, bl, cl, dl, el, F0, X, 0x00000000, 15,  8);

        RIPEMD_ROUND(ar, br, cr, dr, er, F4, X, 0x50a28be6,  5,  8);
        RIPEMD_ROUND(er, ar, br, cr, dr, F4, X, 0x50a28be6, 14,  9);
        RIPEMD_ROUND(dr, er, ar, br, cr, F4, X, 0x50a28be6,  7,  9);
        RIPEMD_ROUND(cr, dr, er, ar, br, F4, X, 0x50a28be6,  0, 11);
        RIPEMD_ROUND(br, cr, dr, er, ar, F4, X, 0x50a28be6,  9, 13);
        RIPEMD_ROUND(ar, br, cr, dr, er, F4, X, 0x50a28be6,  2, 15);
        RIPEMD_ROUND(er, ar, br, cr, dr, F4, X, 0x50a28be6, 11, 15);
        RIPEMD_ROUND(dr, er, ar, br, cr, F4, X, 0x50a28be6,  4,  5);
        RIPEMD_ROUND(cr, dr, er, ar, br, F4, X, 0x50a28be6, 13,  7);
        RIPEMD_ROUND(br, cr, dr, er, ar, F4, X, 0x50a28be6,  6,  7);
        RIPEMD_ROUND(ar, br, cr, dr, er, F4, X, 0x50a28be6, 15,  8);
        RIPEMD_ROUND(er, ar, br, cr, dr, F4, X, 0x50a28be6,  8, 11);
        RIPEMD_ROUND(dr, er, ar, br, cr, F4, X, 0x50a28be6,  1, 14);
        RIPEMD_ROUND(cr, dr, er, ar, br, F4, X, 0x50a28be6, 10, 14);
        RIPEMD_ROUND(br, cr, dr, er, ar, F4, X, 0x50a28be6,  3, 12);
        RIPEMD_ROUND(ar, br, cr, dr, er, F4, X, 0x50a28be6, 12,  6);

        // ============================================================
        // Раунды 16-31: левая F1 (K=0x5a827999), правая F3 (K=0x5c4dd124)
        // ============================================================
        RIPEMD_ROUND(el, al, bl, cl, dl, F1, X, 0x5a827999,  7,  7);
        RIPEMD_ROUND(dl, el, al, bl, cl, F1, X, 0x5a827999,  4,  6);
        RIPEMD_ROUND(cl, dl, el, al, bl, F1, X, 0x5a827999, 13,  8);
        RIPEMD_ROUND(bl, cl, dl, el, al, F1, X, 0x5a827999,  1, 13);
        RIPEMD_ROUND(al, bl, cl, dl, el, F1, X, 0x5a827999, 10, 11);
        RIPEMD_ROUND(el, al, bl, cl, dl, F1, X, 0x5a827999,  6,  9);
        RIPEMD_ROUND(dl, el, al, bl, cl, F1, X, 0x5a827999, 15,  7);
        RIPEMD_ROUND(cl, dl, el, al, bl, F1, X, 0x5a827999,  3, 15);
        RIPEMD_ROUND(bl, cl, dl, el, al, F1, X, 0x5a827999, 12,  7);
        RIPEMD_ROUND(al, bl, cl, dl, el, F1, X, 0x5a827999,  0, 12);
        RIPEMD_ROUND(el, al, bl, cl, dl, F1, X, 0x5a827999,  9, 15);
        RIPEMD_ROUND(dl, el, al, bl, cl, F1, X, 0x5a827999,  5,  9);
        RIPEMD_ROUND(cl, dl, el, al, bl, F1, X, 0x5a827999,  2, 11);
        RIPEMD_ROUND(bl, cl, dl, el, al, F1, X, 0x5a827999, 14,  7);
        RIPEMD_ROUND(al, bl, cl, dl, el, F1, X, 0x5a827999, 11, 13);
        RIPEMD_ROUND(el, al, bl, cl, dl, F1, X, 0x5a827999,  8, 12);

        RIPEMD_ROUND(er, ar, br, cr, dr, F3, X, 0x5c4dd124,  6,  9);
        RIPEMD_ROUND(dr, er, ar, br, cr, F3, X, 0x5c4dd124, 11, 13);
        RIPEMD_ROUND(cr, dr, er, ar, br, F3, X, 0x5c4dd124,  3, 15);
        RIPEMD_ROUND(br, cr, dr, er, ar, F3, X, 0x5c4dd124,  7,  7);
        RIPEMD_ROUND(ar, br, cr, dr, er, F3, X, 0x5c4dd124,  0, 12);
        RIPEMD_ROUND(er, ar, br, cr, dr, F3, X, 0x5c4dd124, 13,  8);
        RIPEMD_ROUND(dr, er, ar, br, cr, F3, X, 0x5c4dd124,  5,  9);
        RIPEMD_ROUND(cr, dr, er, ar, br, F3, X, 0x5c4dd124, 10, 11);
        RIPEMD_ROUND(br, cr, dr, er, ar, F3, X, 0x5c4dd124, 14,  7);
        RIPEMD_ROUND(ar, br, cr, dr, er, F3, X, 0x5c4dd124, 15,  7);
        RIPEMD_ROUND(er, ar, br, cr, dr, F3, X, 0x5c4dd124,  8, 12);
        RIPEMD_ROUND(dr, er, ar, br, cr, F3, X, 0x5c4dd124, 12,  7);
        RIPEMD_ROUND(cr, dr, er, ar, br, F3, X, 0x5c4dd124,  4,  6);
        RIPEMD_ROUND(br, cr, dr, er, ar, F3, X, 0x5c4dd124,  9, 15);
        RIPEMD_ROUND(ar, br, cr, dr, er, F3, X, 0x5c4dd124,  1, 13);
        RIPEMD_ROUND(er, ar, br, cr, dr, F3, X, 0x5c4dd124,  2, 11);

        // ============================================================
        // Раунды 32-47: левая F2 (K=0x6ed9eba1), правая F2 (K=0x6d703ef3)
        // ============================================================
        RIPEMD_ROUND(dl, el, al, bl, cl, F2, X, 0x6ed9eba1,  3, 11);
        RIPEMD_ROUND(cl, dl, el, al, bl, F2, X, 0x6ed9eba1, 10, 13);
        RIPEMD_ROUND(bl, cl, dl, el, al, F2, X, 0x6ed9eba1, 14,  6);
        RIPEMD_ROUND(al, bl, cl, dl, el, F2, X, 0x6ed9eba1,  4,  7);
        RIPEMD_ROUND(el, al, bl, cl, dl, F2, X, 0x6ed9eba1,  9, 14);
        RIPEMD_ROUND(dl, el, al, bl, cl, F2, X, 0x6ed9eba1, 15,  9);
        RIPEMD_ROUND(cl, dl, el, al, bl, F2, X, 0x6ed9eba1,  8, 13);
        RIPEMD_ROUND(bl, cl, dl, el, al, F2, X, 0x6ed9eba1,  1, 15);
        RIPEMD_ROUND(al, bl, cl, dl, el, F2, X, 0x6ed9eba1,  2, 14);
        RIPEMD_ROUND(el, al, bl, cl, dl, F2, X, 0x6ed9eba1,  7,  8);
        RIPEMD_ROUND(dl, el, al, bl, cl, F2, X, 0x6ed9eba1,  0, 13);
        RIPEMD_ROUND(cl, dl, el, al, bl, F2, X, 0x6ed9eba1,  6,  6);
        RIPEMD_ROUND(bl, cl, dl, el, al, F2, X, 0x6ed9eba1, 13,  5);
        RIPEMD_ROUND(al, bl, cl, dl, el, F2, X, 0x6ed9eba1, 11, 12);
        RIPEMD_ROUND(el, al, bl, cl, dl, F2, X, 0x6ed9eba1,  5,  7);
        RIPEMD_ROUND(dl, el, al, bl, cl, F2, X, 0x6ed9eba1, 12,  5);

        RIPEMD_ROUND(dr, er, ar, br, cr, F2, X, 0x6d703ef3, 15,  9);
        RIPEMD_ROUND(cr, dr, er, ar, br, F2, X, 0x6d703ef3,  5,  7);
        RIPEMD_ROUND(br, cr, dr, er, ar, F2, X, 0x6d703ef3,  1, 15);
        RIPEMD_ROUND(ar, br, cr, dr, er, F2, X, 0x6d703ef3,  3, 11);
        RIPEMD_ROUND(er, ar, br, cr, dr, F2, X, 0x6d703ef3,  7,  8);
        RIPEMD_ROUND(dr, er, ar, br, cr, F2, X, 0x6d703ef3, 14,  6);
        RIPEMD_ROUND(cr, dr, er, ar, br, F2, X, 0x6d703ef3,  6,  6);
        RIPEMD_ROUND(br, cr, dr, er, ar, F2, X, 0x6d703ef3,  9, 14);
        RIPEMD_ROUND(ar, br, cr, dr, er, F2, X, 0x6d703ef3, 11, 12);
        RIPEMD_ROUND(er, ar, br, cr, dr, F2, X, 0x6d703ef3,  8, 13);
        RIPEMD_ROUND(dr, er, ar, br, cr, F2, X, 0x6d703ef3, 12,  5);
        RIPEMD_ROUND(cr, dr, er, ar, br, F2, X, 0x6d703ef3,  2, 14);
        RIPEMD_ROUND(br, cr, dr, er, ar, F2, X, 0x6d703ef3, 10, 13);
        RIPEMD_ROUND(ar, br, cr, dr, er, F2, X, 0x6d703ef3,  0, 13);
        RIPEMD_ROUND(er, ar, br, cr, dr, F2, X, 0x6d703ef3,  4,  7);
        RIPEMD_ROUND(dr, er, ar, br, cr, F2, X, 0x6d703ef3, 13,  5);

        // ============================================================
        // Раунды 48-63: левая F3 (K=0x8f1bbcdc), правая F1 (K=0x7a6d76e9)
        // ============================================================
        RIPEMD_ROUND(cl, dl, el, al, bl, F3, X, 0x8f1bbcdc,  1, 11);
        RIPEMD_ROUND(bl, cl, dl, el, al, F3, X, 0x8f1bbcdc,  9, 12);
        RIPEMD_ROUND(al, bl, cl, dl, el, F3, X, 0x8f1bbcdc, 11, 14);
        RIPEMD_ROUND(el, al, bl, cl, dl, F3, X, 0x8f1bbcdc, 10, 15);
        RIPEMD_ROUND(dl, el, al, bl, cl, F3, X, 0x8f1bbcdc,  0, 14);
        RIPEMD_ROUND(cl, dl, el, al, bl, F3, X, 0x8f1bbcdc,  8, 15);
        RIPEMD_ROUND(bl, cl, dl, el, al, F3, X, 0x8f1bbcdc, 12,  9);
        RIPEMD_ROUND(al, bl, cl, dl, el, F3, X, 0x8f1bbcdc,  4,  8);
        RIPEMD_ROUND(el, al, bl, cl, dl, F3, X, 0x8f1bbcdc, 13,  9);
        RIPEMD_ROUND(dl, el, al, bl, cl, F3, X, 0x8f1bbcdc,  3, 14);
        RIPEMD_ROUND(cl, dl, el, al, bl, F3, X, 0x8f1bbcdc,  7,  5);
        RIPEMD_ROUND(bl, cl, dl, el, al, F3, X, 0x8f1bbcdc, 15,  6);
        RIPEMD_ROUND(al, bl, cl, dl, el, F3, X, 0x8f1bbcdc, 14,  8);
        RIPEMD_ROUND(el, al, bl, cl, dl, F3, X, 0x8f1bbcdc,  5,  6);
        RIPEMD_ROUND(dl, el, al, bl, cl, F3, X, 0x8f1bbcdc,  6,  5);
        RIPEMD_ROUND(cl, dl, el, al, bl, F3, X, 0x8f1bbcdc,  2, 12);

        RIPEMD_ROUND(cr, dr, er, ar, br, F1, X, 0x7a6d76e9,  8, 15);
        RIPEMD_ROUND(br, cr, dr, er, ar, F1, X, 0x7a6d76e9,  6,  5);
        RIPEMD_ROUND(ar, br, cr, dr, er, F1, X, 0x7a6d76e9,  4,  8);
        RIPEMD_ROUND(er, ar, br, cr, dr, F1, X, 0x7a6d76e9,  1, 11);
        RIPEMD_ROUND(dr, er, ar, br, cr, F1, X, 0x7a6d76e9,  3, 14);
        RIPEMD_ROUND(cr, dr, er, ar, br, F1, X, 0x7a6d76e9, 11, 14);
        RIPEMD_ROUND(br, cr, dr, er, ar, F1, X, 0x7a6d76e9, 15,  6);
        RIPEMD_ROUND(ar, br, cr, dr, er, F1, X, 0x7a6d76e9,  0, 14);
        RIPEMD_ROUND(er, ar, br, cr, dr, F1, X, 0x7a6d76e9,  5,  6);
        RIPEMD_ROUND(dr, er, ar, br, cr, F1, X, 0x7a6d76e9, 12,  9);
        RIPEMD_ROUND(cr, dr, er, ar, br, F1, X, 0x7a6d76e9,  2, 12);
        RIPEMD_ROUND(br, cr, dr, er, ar, F1, X, 0x7a6d76e9, 13,  9);
        RIPEMD_ROUND(ar, br, cr, dr, er, F1, X, 0x7a6d76e9,  9, 12);
        RIPEMD_ROUND(er, ar, br, cr, dr, F1, X, 0x7a6d76e9,  7,  5);
        RIPEMD_ROUND(dr, er, ar, br, cr, F1, X, 0x7a6d76e9, 10, 15);
        RIPEMD_ROUND(cr, dr, er, ar, br, F1, X, 0x7a6d76e9, 14,  8);

        // ============================================================
        // Раунды 64-79: левая F4 (K=0xa953fd4e), правая F0 (K=0)
        // ============================================================
        RIPEMD_ROUND(bl, cl, dl, el, al, F4, X, 0xa953fd4e,  4,  9);
        RIPEMD_ROUND(al, bl, cl, dl, el, F4, X, 0xa953fd4e,  0, 15);
        RIPEMD_ROUND(el, al, bl, cl, dl, F4, X, 0xa953fd4e,  5,  5);
        RIPEMD_ROUND(dl, el, al, bl, cl, F4, X, 0xa953fd4e,  9, 11);
        RIPEMD_ROUND(cl, dl, el, al, bl, F4, X, 0xa953fd4e,  7,  6);
        RIPEMD_ROUND(bl, cl, dl, el, al, F4, X, 0xa953fd4e, 12,  8);
        RIPEMD_ROUND(al, bl, cl, dl, el, F4, X, 0xa953fd4e,  2, 13);
        RIPEMD_ROUND(el, al, bl, cl, dl, F4, X, 0xa953fd4e, 10, 12);
        RIPEMD_ROUND(dl, el, al, bl, cl, F4, X, 0xa953fd4e, 14,  5);
        RIPEMD_ROUND(cl, dl, el, al, bl, F4, X, 0xa953fd4e,  1, 12);
        RIPEMD_ROUND(bl, cl, dl, el, al, F4, X, 0xa953fd4e,  3, 13);
        RIPEMD_ROUND(al, bl, cl, dl, el, F4, X, 0xa953fd4e,  8, 14);
        RIPEMD_ROUND(el, al, bl, cl, dl, F4, X, 0xa953fd4e, 11, 11);
        RIPEMD_ROUND(dl, el, al, bl, cl, F4, X, 0xa953fd4e,  6,  8);
        RIPEMD_ROUND(cl, dl, el, al, bl, F4, X, 0xa953fd4e, 15,  5);
        RIPEMD_ROUND(bl, cl, dl, el, al, F4, X, 0xa953fd4e, 13,  6);

        RIPEMD_ROUND(br, cr, dr, er, ar, F0, X, 0x00000000, 12,  8);
        RIPEMD_ROUND(ar, br, cr, dr, er, F0, X, 0x00000000, 15,  5);
        RIPEMD_ROUND(er, ar, br, cr, dr, F0, X, 0x00000000, 10, 12);
        RIPEMD_ROUND(dr, er, ar, br, cr, F0, X, 0x00000000,  4,  9);
        RIPEMD_ROUND(cr, dr, er, ar, br, F0, X, 0x00000000,  1, 12);
        RIPEMD_ROUND(br, cr, dr, er, ar, F0, X, 0x00000000,  5,  5);
        RIPEMD_ROUND(ar, br, cr, dr, er, F0, X, 0x00000000,  8, 14);
        RIPEMD_ROUND(er, ar, br, cr, dr, F0, X, 0x00000000,  7,  6);
        RIPEMD_ROUND(dr, er, ar, br, cr, F0, X, 0x00000000,  6,  8);
        RIPEMD_ROUND(cr, dr, er, ar, br, F0, X, 0x00000000,  2, 13);
        RIPEMD_ROUND(br, cr, dr, er, ar, F0, X, 0x00000000, 13,  6);
        RIPEMD_ROUND(ar, br, cr, dr, er, F0, X, 0x00000000, 14,  5);
        RIPEMD_ROUND(er, ar, br, cr, dr, F0, X, 0x00000000,  0, 15);
        RIPEMD_ROUND(dr, er, ar, br, cr, F0, X, 0x00000000,  3, 13);
        RIPEMD_ROUND(cr, dr, er, ar, br, F0, X, 0x00000000,  9, 11);
        RIPEMD_ROUND(br, cr, dr, er, ar, F0, X, 0x00000000, 11, 11);

        // ============================================================
        // Финальное смешивание двух ветвей
        // ============================================================
        uint32_t t = h1 + cl + dr;
        h1 = h2 + dl + er;
        h2 = h3 + el + ar;
        h3 = h4 + al + br;
        h4 = h0 + bl + cr;
        h0 = t;
    }

    delete[] block;

    // Вывод в little-endian (RIPEMD-160 использует LE)
    out[ 0] = (uint8_t)(h0);
    out[ 1] = (uint8_t)(h0 >> 8);
    out[ 2] = (uint8_t)(h0 >> 16);
    out[ 3] = (uint8_t)(h0 >> 24);
    out[ 4] = (uint8_t)(h1);
    out[ 5] = (uint8_t)(h1 >> 8);
    out[ 6] = (uint8_t)(h1 >> 16);
    out[ 7] = (uint8_t)(h1 >> 24);
    out[ 8] = (uint8_t)(h2);
    out[ 9] = (uint8_t)(h2 >> 8);
    out[10] = (uint8_t)(h2 >> 16);
    out[11] = (uint8_t)(h2 >> 24);
    out[12] = (uint8_t)(h3);
    out[13] = (uint8_t)(h3 >> 8);
    out[14] = (uint8_t)(h3 >> 16);
    out[15] = (uint8_t)(h3 >> 24);
    out[16] = (uint8_t)(h4);
    out[17] = (uint8_t)(h4 >> 8);
    out[18] = (uint8_t)(h4 >> 16);
    out[19] = (uint8_t)(h4 >> 24);
}

// ============================================================================
// HASH160 = RIPEMD160(SHA256(data))
// Это стандартная операция Bitcoin для получения идентификатора адреса
// ============================================================================
void hash160(const uint8_t* data, size_t len, uint8_t out[20])
{
    uint8_t sha[32];
    sha256(data, len, sha);
    ripemd160(sha, 32, out);
}