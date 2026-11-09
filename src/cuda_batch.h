#pragma once
#include <cstdint>
#include <vector>

class CUDABatchProcessor {
public:
    CUDABatchProcessor();
    ~CUDABatchProcessor();

    void Initialize(uint64_t max_batch_size);

    // Принимает массив координат X и Y (по 32 байта каждая) и возвращает HASH160 (20 байт)
    void ComputeHash160Batch(
        const uint8_t* pubkeys_x,
        const uint8_t* pubkeys_y,
        uint64_t num_keys,
        uint8_t* hash160s);

    bool IsCpuFallback() const { return cpu_fallback; }

private:
    void ComputeHash160BatchCpu(
        const uint8_t* pubkeys_x,
        const uint8_t* pubkeys_y,
        uint64_t num_keys,
        uint8_t* hash160s);

    uint8_t* d_pubkeys_x;
    uint8_t* d_pubkeys_y;
    uint8_t* d_hash160s;
    uint64_t max_batch_size;
    bool initialized;
    bool cpu_fallback;
};

extern CUDABatchProcessor g_cuda_batch;