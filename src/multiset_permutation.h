#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

// Инициализация таблицы факториалов (вызвать один раз при старте)
void InitFactorials();

// === БЫСТРЫЕ БАЙТОВЫЕ ВЕРСИИ (для горячего цикла) ===

// Подсчёт уникальных перестановок мультимножества байт
uint64_t CountUniquePermutationsBytes(const uint8_t* symbols, size_t symLen);

// Получить перестановку по индексу (заполняет out массивом байт длиной symLen)
void GetUniquePermutationBytes(const uint8_t* symbols, size_t symLen, uint64_t idx, uint8_t* out);

// Конвертация 64 nibble (значения 0-15) в 32 байта (Big-Endian)
void NibblesToBytes32(const uint8_t* nibbles64, uint8_t* out32);

// === STRING-ВЕРСИИ (для совместимости с GUI, вызываются один раз) ===
uint64_t CountUniquePermutations(const std::string& symbols);
std::string GetUniquePermutationByIndex(const std::string& symbols, uint64_t idx);