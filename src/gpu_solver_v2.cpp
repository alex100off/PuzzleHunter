#include "gpu_solver_v2.h"
#include "hash.h"
#include "targets.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <iomanip>
#include <csignal>
#include "multiset_permutation.h"
#include "bitcoin.h"
#include "cuda_batch.h"

#ifdef _WIN32
#include <windows.h>
#endif

static GPUSolverV2* g_solver = nullptr;
static std::string g_symbols;
static std::atomic<uint64_t> g_currentIndex{0};

#ifdef _WIN32
BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT || ctrlType == CTRL_BREAK_EVENT) {
        std::cout << "\n\n[Console Handler] Stop signal received. Saving progress..." << std::endl;
        if (g_solver && !g_symbols.empty()) {
            g_solver->SaveProgress(g_symbols, g_currentIndex.load());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        exit(0);
    }
    return TRUE;
}
#endif

GPUSolverV2::GPUSolverV2() : targetsFinalized(false) {
    secp.Init();
    std::cout << "GPU Solver V2 initialized (GPU-accelerated mode)" << std::endl;
    
    g_solver = this;
    std::signal(SIGINT, [](int) { ConsoleCtrlHandler(CTRL_C_EVENT); });
    std::signal(SIGTERM, [](int) { ConsoleCtrlHandler(CTRL_CLOSE_EVENT); });
    #ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    #endif
}

GPUSolverV2::~GPUSolverV2() {}

void GPUSolverV2::AddTarget(const uint8_t hash160[20]) {
    if (targetsFinalized) return;
	  bool allZero = true;
    for (int i = 0; i < 20; ++i) {
        if (hash160[i] != 0) { allZero = false; break; }
    }
    if (allZero)
    for (const auto& existing : targetHashes) {
        if (std::memcmp(existing.data(), hash160, 20) == 0) return;
		    }
    std::vector<uint8_t> hash(20);
    std::memcpy(hash.data(), hash160, 20);
    targetHashes.push_back(hash);
    
    prefix_t prefix = *(prefix_t*)hash160;
    for (prefix_t p : prefixes) {
        if (p == prefix) return;
    }
    prefixes.push_back(prefix);
}

void GPUSolverV2::SaveProgress(const std::string& symbols, uint64_t index) {
    std::ofstream file("progress.dat");
    if (file.is_open()) {
        file << symbols << "\n" << index << "\n";
        file.close();
        std::cout << "[Progress saved at index " << index << "]" << std::endl;
    }
}

bool GPUSolverV2::LoadProgress(const std::string& symbols, uint64_t& startIndex) {
    std::ifstream file("progress.dat");
    if (!file.is_open()) return false;
    
    std::string savedSymbols;
    uint64_t savedIndex;
    
    if (file >> savedSymbols >> savedIndex) {
        file.close();
        if (savedSymbols == symbols) {
            startIndex = savedIndex;
            std::cout << "Resuming from index " << startIndex << std::endl;
            return true;
        }
    }
    file.close();
    return false;
}

void GPUSolverV2::ScanPermutations(const std::string& symbols, uint64_t count, int numThreads, std::atomic<bool>& running) {
    try {
        InitSecp256k1();
        InitFactorials();  // Предвычисляем факториалы ОДИН РАЗ
        g_symbols = symbols;
        
        int actualThreads = numThreads < 2 ? 2 : numThreads;
        if (actualThreads != numThreads) {
            std::cout << "[Warning] Increased threads to " << actualThreads << " for better GPU utilization." << std::endl;
        }
        
        std::cout << "ScanPermutations: Starting..." << std::endl;
        std::cout << "Total permutations to scan: " << count << std::endl;
        
        uint64_t startIndex = 0;
        if (LoadProgress(symbols, startIndex)) {
            std::cout << "Continuing from previous session..." << std::endl;
        }
        
        if (startIndex > 0) {
            if (startIndex >= count) {
                std::cout << "Already completed all permutations!" << std::endl;
                return;
            }
            count -= startIndex;
            std::cout << "Remaining permutations: " << count << std::endl;
        }
        
        SaveProgress(symbols, startIndex);
        std::cout << "Loaded " << targetHashes.size() << " targets." << std::endl;
        
        if (!targetsFinalized && !prefixes.empty()) {
            g_cuda_batch.Initialize(100000);
            targetsFinalized = true;
            std::cout << "Loaded " << prefixes.size() << " unique target prefixes." << std::endl;
        }
        
        std::cout << "Using " << actualThreads << " CPU threads for EC math + GPU for hashing." << std::endl;
        
        // === ОПТИМИЗАЦИЯ: парсим символы в nibble ОДИН РАЗ до цикла ===
        uint8_t symNibbles[64];
        size_t symLen = symbols.length();
        for (size_t i = 0; i < symLen; i++) {
            char c = symbols[i];
            if (c >= '0' && c <= '9') symNibbles[i] = c - '0';
            else if (c >= 'a' && c <= 'f') symNibbles[i] = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') symNibbles[i] = c - 'A' + 10;
            else symNibbles[i] = 0;
        }
        size_t leadingZeros = 64 - symLen;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        std::atomic<uint64_t> processed{0};
        std::atomic<uint64_t> matchesFound{0};
        std::mutex logMutex;
        
        uint64_t perThread = count / actualThreads;
        if (perThread == 0) perThread = 1;
        
        const int GPU_BATCH_SIZE = 5000;
        
        for (int t = 0; t < actualThreads; ++t) {
            uint64_t startIdx = t * perThread;
            uint64_t threadCount = (t == actualThreads - 1) ? (count - startIdx) : perThread;
            
            threads.emplace_back([this, symNibbles, symLen, leadingZeros, startIdx, threadCount, count, startIndex, &processed, &matchesFound, &startTime, &logMutex, GPU_BATCH_SIZE, &running]() {
                Secp256K1 localSecp;
                localSecp.Init();
                
                std::vector<uint8_t> pubkeyX(GPU_BATCH_SIZE * 32);
                std::vector<uint8_t> pubkeyY(GPU_BATCH_SIZE * 32);
                std::vector<uint8_t> hash160s(GPU_BATCH_SIZE * 20);
                std::vector<Int> privKeys(GPU_BATCH_SIZE);
                
                int batchIdx = 0;
                
                // СТЕКОВЫЕ МАССИВЫ для горячего цикла (без динамической памяти)
                uint8_t permNibbles[64];
                uint8_t fullNibbles[64];
                uint8_t privKeyBytes[32];
                
                for (uint64_t i = 0; i < threadCount; ++i) {
                    if (!running.load()) {
                        std::cout << "[Thread " << std::this_thread::get_id() << "] Stopping..." << std::endl;
                        break;
                    }
                    
                    uint64_t idx = startIndex + startIdx + i;
                    
                    // 1. Генерируем перестановку nibble (без std::string!)
                    GetUniquePermutationBytes(symNibbles, symLen, idx, permNibbles);
                    
                    // 2. Собираем полный 64-nibble ключ с ведущими нулями
                    memset(fullNibbles, 0, leadingZeros);
                    memcpy(fullNibbles + leadingZeros, permNibbles, symLen);
                    
                    // 3. Конвертируем 64 nibble → 32 байта
                    NibblesToBytes32(fullNibbles, privKeyBytes);
                    
                    // 4. Загружаем приватный ключ напрямую из байт (без парсинга hex-строки!)
                    Int privKey;
                    privKey.Set32Bytes(privKeyBytes);
                    
                    // 5. Вычисляем публичный ключ
                    Point p = localSecp.ComputePublicKey(&privKey);
                    
                    // 6. Копируем координаты в буфер GPU
                    std::memcpy(pubkeyX.data() + batchIdx * 32, p.x.bits64, 32);
                    std::memcpy(pubkeyY.data() + batchIdx * 32, p.y.bits64, 32);
                    
                    privKeys[batchIdx] = privKey;
                    batchIdx++;
                    
                    if (batchIdx == GPU_BATCH_SIZE) {
                        g_cuda_batch.ComputeHash160Batch(
                            pubkeyX.data(),
                            pubkeyY.data(),
                            GPU_BATCH_SIZE,
                            hash160s.data()
                        );
                        
                        for (int j = 0; j < GPU_BATCH_SIZE; ++j) {
                            processed++;
                            for (const auto& targetHash : targetHashes) {
                                if (std::memcmp(hash160s.data() + j * 20, targetHash.data(), 20) == 0) {
                                    matchesFound++;
                                    // Используем GetBase16() только при совпадении (редко)
                                    std::string privHex = privKeys[j].GetBase16();
                                    
                                    {
                                        std::lock_guard<std::mutex> lock(logMutex);
                                        std::cout << "\n!!! FOUND MATCH !!!" << std::endl;
                                        std::cout << "PrivKey: " << privHex << std::endl;
                                    }
                                    
                                    std::ofstream log("found_keys.log", std::ios::app);
                                    if (log.is_open()) {
                                        log << "FOUND\nPrivKey: " << privHex << "\n---\n";
                                        log.close();
                                    }
                                }
                            }
                        }
                        batchIdx = 0;
                        
                        uint64_t currentTotal = processed.load();
                        if (currentTotal % 1000000 == 0) {
                            g_currentIndex.store(startIndex + currentTotal);
                            auto currentTime = std::chrono::high_resolution_clock::now();
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
                            if (duration > 0) {
                                double speed = (double)currentTotal * 1000.0 / (double)duration;
                                uint64_t remaining = count - currentTotal;
                                double remainingSeconds = (double)remaining / speed;
                                int hours = (int)(remainingSeconds / 3600);
                                int minutes = (int)((remainingSeconds - hours * 3600) / 60);
                                int seconds = (int)(remainingSeconds - hours * 3600 - minutes * 60);
                                
                                std::lock_guard<std::mutex> lock(logMutex);
                                std::cout << "Processed: " << currentTotal << " (" << speed << " keys/sec)" << std::endl;
                                std::cout << "ETA: " << hours << "h " << minutes << "m " << seconds << "s" << std::endl;
                                SaveProgress(g_symbols, startIndex + currentTotal);
                            }
                        }
                    }
                }
                
                if (batchIdx > 0) {
                    g_cuda_batch.ComputeHash160Batch(pubkeyX.data(), pubkeyY.data(), batchIdx, hash160s.data());
                    for (int j = 0; j < batchIdx; ++j) {
                        processed++;
                        for (const auto& targetHash : targetHashes) {
                            if (std::memcmp(hash160s.data() + j * 20, targetHash.data(), 20) == 0) {
                                matchesFound++;
                                std::string privHex = privKeys[j].GetBase16();
                                std::lock_guard<std::mutex> lock(logMutex);
                                std::cout << "\n!!! FOUND MATCH !!!" << std::endl;
                                std::cout << "PrivKey: " << privHex << std::endl;
                            }
                        }
                    }
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        uint64_t finalIndex = startIndex + processed.load();
        SaveProgress(g_symbols, finalIndex);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Solver finished." << std::endl;
        std::cout << "Processed: " << processed << " keys in " << duration << " ms" << std::endl;
        if (duration > 0) {
            double speed = (double)processed * 1000.0 / (double)duration;
            std::cout << "Speed: " << speed << " keys/sec" << std::endl;
        }
        std::cout << "Total matches found: " << matchesFound << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in ScanPermutations: " << e.what() << std::endl;
        throw;
    }
}