#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <atomic>
#include "Int.h"
#include "SECP256K1.h"
#include "../external/vanity/GPU/GPUEngine.h"

class GPUSolverV2 {
public:
    GPUSolverV2();
    ~GPUSolverV2();

    void AddTarget(const uint8_t hash160[20]);
	void ScanPermutations(const std::string& symbols, uint64_t count, int numThreads, std::atomic<bool>& running);
    
    void SaveProgress(const std::string& symbols, uint64_t index);
    bool LoadProgress(const std::string& symbols, uint64_t& startIndex);
    
    std::vector<Int> foundKeys;
    Secp256K1 secp;

private:
    std::vector<prefix_t> prefixes;
    std::vector<std::vector<uint8_t>> targetHashes;
    bool targetsFinalized;
};