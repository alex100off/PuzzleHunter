#include "multiset_permutation.h"
#include <cstring>
#include <algorithm>

// Таблица факториалов [0..64]
static uint64_t g_factorials[65];
static bool g_factorialsInit = false;

void InitFactorials() {
    if (g_factorialsInit) return;
    g_factorials[0] = 1;
    for (int i = 1; i <= 64; i++) {
        g_factorials[i] = g_factorials[i-1] * (uint64_t)i;
    }
    g_factorialsInit = true;
}

// Вычисление N! / (count_1! * count_2! * ... * count_k!)
static uint64_t Multinomial(size_t N, const uint32_t* counts, int numSymbols) {
    uint64_t result = g_factorials[N];
    for (int i = 0; i < numSymbols; i++) {
        if (counts[i] > 1) {
            result /= g_factorials[counts[i]];
        }
    }
    return result;
}

uint64_t CountUniquePermutationsBytes(const uint8_t* symbols, size_t symLen) {
    if (symLen == 0 || symLen > 64) return 0;
    
    uint32_t counts[256] = {0};
    for (size_t i = 0; i < symLen; i++) {
        counts[symbols[i]]++;
    }
    
    int numSymbols = 0;
    uint32_t nonZeroCounts[64];
    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            nonZeroCounts[numSymbols++] = counts[i];
        }
    }
    
    return Multinomial(symLen, nonZeroCounts, numSymbols);
}

void GetUniquePermutationBytes(const uint8_t* symbols, size_t symLen, uint64_t idx, uint8_t* out) {
    // Сортируем символы для детерминированного порядка
    uint8_t sorted[64];
    memcpy(sorted, symbols, symLen);
    std::sort(sorted, sorted + symLen);
    
    // Считаем частоты уникальных символов
    uint8_t unique[64];
    uint32_t counts[64];
    int numUnique = 0;
    
    for (size_t i = 0; i < symLen; ) {
        uint8_t val = sorted[i];
        uint32_t cnt = 0;
        while (i < symLen && sorted[i] == val) {
            cnt++;
            i++;
        }
        unique[numUnique] = val;
        counts[numUnique] = cnt;
        numUnique++;
    }
    
    size_t remaining = symLen;
    
    for (size_t pos = 0; pos < symLen; pos++) {
        remaining--;
        for (int u = 0; u < numUnique; u++) {
            if (counts[u] == 0) continue;
            
            // Сколько перестановок, если поставить unique[u] на позицию pos
            counts[u]--;
            uint64_t perms = Multinomial(remaining, counts, numUnique);
            
            if (idx < perms) {
                out[pos] = unique[u];
                break;
            } else {
                idx -= perms;
                counts[u]++; // откат
            }
        }
    }
}

void NibblesToBytes32(const uint8_t* nibbles64, uint8_t* out32) {
    for (size_t i = 0; i < 32; i++) {
        out32[i] = (nibbles64[i * 2] << 4) | nibbles64[i * 2 + 1];
    }
}

// === STRING-ВЕРСИИ (для совместимости) ===

uint64_t CountUniquePermutations(const std::string& symbols) {
    if (symbols.empty()) return 0;
    
    // Конвертируем hex-символы в nibble
    uint8_t nibbles[64];
    size_t len = symbols.length();
    for (size_t i = 0; i < len; i++) {
        char c = symbols[i];
        if (c >= '0' && c <= '9') nibbles[i] = c - '0';
        else if (c >= 'a' && c <= 'f') nibbles[i] = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') nibbles[i] = c - 'A' + 10;
        else nibbles[i] = 0;
    }
    
    InitFactorials();
    return CountUniquePermutationsBytes(nibbles, len);
}

std::string GetUniquePermutationByIndex(const std::string& symbols, uint64_t idx) {
    InitFactorials();
    
    uint8_t nibbles[64];
    size_t len = symbols.length();
    for (size_t i = 0; i < len; i++) {
        char c = symbols[i];
        if (c >= '0' && c <= '9') nibbles[i] = c - '0';
        else if (c >= 'a' && c <= 'f') nibbles[i] = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') nibbles[i] = c - 'A' + 10;
        else nibbles[i] = 0;
    }
    
    uint8_t out[64];
    GetUniquePermutationBytes(nibbles, len, idx, out);
    
    // Конвертируем обратно в hex-строку
    static const char hexChars[] = "0123456789abcdef";
    std::string result(len, '0');
    for (size_t i = 0; i < len; i++) {
        result[i] = hexChars[out[i]];
    }
    return result;
}