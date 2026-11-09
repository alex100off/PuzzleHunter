#pragma once
#include "../../src/hash.h"  // Добавь эту строку
#include <cstdint>

// Объявления функций-адаптеров для хеширования
extern "C" {
    // Базовая функция SHA256 (из нашего src/hash.cpp)
    void sha256(const uint8_t* data, size_t len, uint8_t out[32]);
    
    // Специализированные версии для VanitySearch
    void sha256_33(unsigned char *input, unsigned char *digest);
    void sha256_65(unsigned char *input, unsigned char *digest);
    void sha256_checksum(unsigned char *input, int length, unsigned char *checksum);
    void ripemd160_32(unsigned char *input, unsigned char *digest);
    
    // SSE-версии (вызывают скалярную версию 4 раза)
    void sha256sse_1B(unsigned int *i0, unsigned int *i1, unsigned int *i2, unsigned int *i3, 
                      unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3);
    
    void sha256sse_2B(unsigned int *i0, unsigned int *i1, unsigned int *i2, unsigned int *i3, 
                      unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3);
    
    void sha256sse_checksum(unsigned int *i0, unsigned int *i1, unsigned int *i2, unsigned int *i3, 
                            unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3);
    
    void ripemd160sse_32(unsigned char *i0, unsigned char *i1, unsigned char *i2, unsigned char *i3, 
                         unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3);
}