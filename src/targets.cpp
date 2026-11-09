#include "targets.h"
#include "bitcoin.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

// ============================================================================
// Глобальный список целей.
// В будущем можно загружать из файла puzzles.txt,
// но пока захардкодим для надёжности.
// ============================================================================
static std::vector<Target> g_targets;

// ============================================================================
// Вспомогательная функция: конвертация hex-строки (40 символов) в 20 байт.
// Используется только при инициализации, не в горячем цикле.
// ============================================================================
static bool HexToHash160(const std::string& hex, uint8_t out[20])
{
    if (hex.size() != 40) return false;
    
    for (size_t i = 0; i < 20; ++i) {
        unsigned int byte;
        std::istringstream iss(hex.substr(i * 2, 2));
        iss >> std::hex >> byte;
        if (iss.fail()) return false;
        out[i] = (uint8_t)byte;
    }
    return true;
}

void InitTargets()
{
    g_targets.clear();
    
    // Тестовая цель - HASH160 от перестановки с индексом 1000000
   {
    Target t;
    t.name = "TestKey (perm index 1000000)";
    if (HexToHash160("f6f5431d25bbf7b12e8add9af5e3475c44a0a5b8", t.hash160)) {
        g_targets.push_back(t);
      }
    }
    
    // ========================================================================
    // РЕАЛЬНЫЕ ЦЕЛИ: Bitcoin Puzzle 71-79
    // HASH160 публичных ключей из блокчейна (открытые данные).
    // Эти адреса известны и публикуются на btcdir.org и подобных ресурсах.
    // ========================================================================
    
    // Puzzle 71
    {
        Target t;
        t.name = "Puzzle 71";
        if (HexToHash160("f6f5431d25bbf7b12e8add9af5e3475c44a0a5b8", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    // Puzzle 72
    {
        Target t;
        t.name = "Puzzle 72";
        if (HexToHash160("bf7413e8df4e7a34ce9dc13e2f2648783ec54adb", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    // Puzzle 73
    {
        Target t;
        t.name = "Puzzle 73";
        if (HexToHash160("105b7f253f0ebd7843adaebbd805c944bfb863e4", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    // Puzzle 74
    {
        Target t;
        t.name = "Puzzle 74";
        if (HexToHash160("9f1adb20baeacc38b3f49f3df6906a0e48f2df3d", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    // Puzzle 75
    {
        Target t;
        t.name = "Puzzle 75";
        if (HexToHash160("badf8b0d34289e679ec65c6c61d3a974353be5cf", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    // Puzzle 76
    {
        Target t;
        t.name = "Puzzle 76";
        if (HexToHash160("86f9fea5cdecf033161dd2f8f8560768ae0a6d14", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    // Puzzle 77
    {
        Target t;
        t.name = "Puzzle 77";
        if (HexToHash160("783c138ac81f6a52398564bb17455576e8525b29", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    // Puzzle 78
    {
        Target t;
        t.name = "Puzzle 78";
        if (HexToHash160("35003c3ef8759c92092f8488fca59a042859018c", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    // Puzzle 79
    {
        Target t;
        t.name = "Puzzle 79";
        if (HexToHash160("67671d5490c272e3ab7ddd34030d587738df33da", t.hash160)) {
            g_targets.push_back(t);
        }
    }
    
    std::cout << "Loaded " << g_targets.size() << " targets\n";
}

bool CheckHash160(const uint8_t hash160[20], Target& foundTarget)
{
    // ========================================================================
    // Простое линейное сравнение со всеми целями.
    // Для 9-100 целей это быстрее, чем hash-таблица,
    // потому что нет накладных расходов на хеширование.
    // Если целей станет >1000, заменим на bloom filter или unordered_map.
    // ========================================================================
    for (const auto& target : g_targets) {
        if (std::memcmp(hash160, target.hash160, 20) == 0) {
            foundTarget = target;
            return true;
        }
    }
    return false;
}

void LogFoundKey(
    const Target& target,
    const std::string& permutation,
    const uint8_t pubkey[33],
    const uint8_t hash160[20])
{
    // ========================================================================
    // Записываем найденный ключ в файл found_keys.log.
    // Формат удобен для последующей обработки и проверки.
    // ========================================================================
    std::ofstream log("found_keys.log", std::ios::app);
    if (!log.is_open()) {
        std::cerr << "ERROR: cannot open found_keys.log\n";
        return;
    }
    
    // Текущее время
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    log << "========================================\n";
    log << "FOUND MATCH!\n";
    log << "Time: " << std::ctime(&time);
    log << "Target: " << target.name << "\n";
    log << "Permutation: " << permutation << "\n";
    log << "PubKey: " << BytesToHex(pubkey, 33) << "\n";
    log << "HASH160: " << BytesToHex(hash160, 20) << "\n";
    log << "========================================\n\n";
    
    log.close();
    
    // Также выводим в консоль, чтобы сразу было видно
    std::cout << "\n!!! FOUND MATCH: " << target.name << " !!!\n";
    std::cout << "Permutation: " << permutation << "\n";
    std::cout << "PubKey: " << BytesToHex(pubkey, 33) << "\n";
    std::cout << "HASH160: " << BytesToHex(hash160, 20) << "\n\n";
}

const std::vector<Target>& GetTargets()
{
    return g_targets;
}