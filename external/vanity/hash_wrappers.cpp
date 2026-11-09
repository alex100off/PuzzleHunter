#include "hash_wrappers.h"
#include <cstdint>

extern "C" {

void sha256_33(unsigned char *input, unsigned char *digest) {
    sha256((const uint8_t*)input, 33, (uint8_t*)digest);
}

void sha256_65(unsigned char *input, unsigned char *digest) {
    sha256((const uint8_t*)input, 65, (uint8_t*)digest);
}

void sha256_checksum(unsigned char *input, int length, unsigned char *checksum) {
    uint8_t hash1[32];
    sha256((const uint8_t*)input, (size_t)length, hash1);
    sha256(hash1, 32, (uint8_t*)checksum);
}

void ripemd160_32(unsigned char *input, unsigned char *digest) {
    ripemd160((const uint8_t*)input, 32, (uint8_t*)digest);
}

void sha256sse_1B(unsigned int *i0, unsigned int *i1, unsigned int *i2, unsigned int *i3, 
                  unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3) {
    sha256((const uint8_t*)i0, 33, d0);
    sha256((const uint8_t*)i1, 33, d1);
    sha256((const uint8_t*)i2, 33, d2);
    sha256((const uint8_t*)i3, 33, d3);
}

void sha256sse_2B(unsigned int *i0, unsigned int *i1, unsigned int *i2, unsigned int *i3, 
                  unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3) {
    sha256((const uint8_t*)i0, 65, d0);
    sha256((const uint8_t*)i1, 65, d1);
    sha256((const uint8_t*)i2, 65, d2);
    sha256((const uint8_t*)i3, 65, d3);
}

void sha256sse_checksum(unsigned int *i0, unsigned int *i1, unsigned int *i2, unsigned int *i3, 
                        unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3) {
    uint8_t h0[32], h1[32], h2[32], h3[32];
    sha256((const uint8_t*)i0, 65, h0); sha256(h0, 32, d0);
    sha256((const uint8_t*)i1, 65, h1); sha256(h1, 32, d1);
    sha256((const uint8_t*)i2, 65, h2); sha256(h2, 32, d2);
    sha256((const uint8_t*)i3, 65, h3); sha256(h3, 32, d3);
}

void ripemd160sse_32(unsigned char *i0, unsigned char *i1, unsigned char *i2, unsigned char *i3, 
                     unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3) {
    ripemd160((const uint8_t*)i0, 32, d0);
    ripemd160((const uint8_t*)i1, 32, d1);
    ripemd160((const uint8_t*)i2, 32, d2);
    ripemd160((const uint8_t*)i3, 32, d3);
}

} // extern "C"