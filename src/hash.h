#pragma once
#include <cstdint>
#include <string>

// Функции хеширования с C linkage (для совместимости с VanitySearch)
extern "C" {
    void sha256(const uint8_t* data, size_t len, uint8_t out[32]);
    void ripemd160(const uint8_t* data, size_t len, uint8_t out[20]);
    void hash160(const uint8_t* data, size_t len, uint8_t out[20]);
}

// C++ функции (не требуют extern "C")
std::string BytesToHex(const uint8_t* data, size_t len);
std::string BytesToHash160(const uint8_t hash160[20]);