#include "cuda_batch.h"
#include <cuda_runtime.h>
#include <iostream>
#include <cstring>

// Родное хеширование VanitySearch (GPU)
#include "../external/vanity/GPU/GPUHash.h"

// CPU-классы для фолбэка (явные пути — обход коллизии SECP256K1.h/secp256k1.h на Windows)
#include "../external/vanity/Int.h"
#include "../external/vanity/Point.h"
#include "../external/vanity/SECP256K1.h"
#include "bitcoin.h"

__global__ void hash160_comp_kernel(uint64_t* d_x, uint64_t* d_y, uint8_t* d_hash160, uint64_t num_keys) {
    uint64_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_keys) return;

    uint64_t* x = d_x + (idx * 4);
    uint64_t* y = d_y + (idx * 4);
    uint8_t* hash = d_hash160 + (idx * 20);

    uint8_t isOdd = y[0] & 1;
    _GetHash160Comp(x, isOdd, hash);
}

CUDABatchProcessor::CUDABatchProcessor()
    : d_pubkeys_x(nullptr), d_pubkeys_y(nullptr), d_hash160s(nullptr),
      max_batch_size(0), initialized(false), cpu_fallback(false) {}

CUDABatchProcessor::~CUDABatchProcessor() {
    if (d_pubkeys_x) cudaFree(d_pubkeys_x);
    if (d_pubkeys_y) cudaFree(d_pubkeys_y);
    if (d_hash160s) cudaFree(d_hash160s);
}

void CUDABatchProcessor::Initialize(uint64_t max_batch_size) {
    if (initialized) return;
    this->max_batch_size = max_batch_size;

    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        cpu_fallback = true;
        initialized = true;
        std::cout << "CUDA Batch Processor initialized (CPU fallback mode, max batch: " << max_batch_size << ")" << std::endl;
        return;
    }

    cudaMalloc(&d_pubkeys_x, max_batch_size * 32);
    cudaMalloc(&d_pubkeys_y, max_batch_size * 32);
    cudaMalloc(&d_hash160s, max_batch_size * 20);

    initialized = true;
    std::cout << "CUDA Batch Processor initialized (GPU mode, max batch: " << max_batch_size << ")" << std::endl;
}

void CUDABatchProcessor::ComputeHash160BatchCpu(
    const uint8_t* pubkeys_x,
    const uint8_t* pubkeys_y,
    uint64_t num_keys,
    uint8_t* hash160s)
{
    thread_local Secp256K1 secp;
    thread_local bool secp_ready = false;
    if (!secp_ready) { secp.Init(); secp_ready = true; }

    Int X;
    Int Y;
    Point p;
    for (uint64_t i = 0; i < num_keys; ++i) {
        std::memcpy(X.bits64, pubkeys_x + i * 32, 32);
        X.bits64[4] = 0;
        std::memcpy(Y.bits64, pubkeys_y + i * 32, 32);
        Y.bits64[4] = 0;
        p.x = X;
        p.y = Y;
        p.z.SetInt32(1);
        secp.GetHash160(P2PKH, true, p, hash160s + i * 20);
    }
}

void CUDABatchProcessor::ComputeHash160Batch(
    const uint8_t* pubkeys_x,
    const uint8_t* pubkeys_y,
    uint64_t num_keys,
    uint8_t* hash160s)
{
    if (!initialized || num_keys == 0) return;
    if (num_keys > max_batch_size) {
        std::cerr << "ERROR: batch size " << num_keys << " exceeds max " << max_batch_size << std::endl;
        return;
    }

    if (cpu_fallback) {
        ComputeHash160BatchCpu(pubkeys_x, pubkeys_y, num_keys, hash160s);
        return;
    }

    cudaMemcpy(d_pubkeys_x, pubkeys_x, num_keys * 32, cudaMemcpyHostToDevice);
    cudaMemcpy(d_pubkeys_y, pubkeys_y, num_keys * 32, cudaMemcpyHostToDevice);

    int threadsPerBlock = 256;
    int blocksPerGrid = (int)((num_keys + threadsPerBlock - 1) / threadsPerBlock);

    hash160_comp_kernel<<<blocksPerGrid, threadsPerBlock>>>(
        (uint64_t*)d_pubkeys_x,
        (uint64_t*)d_pubkeys_y,
        d_hash160s,
        num_keys
    );

    // Если ядро не может выполниться на этом GPU (чужая архитектура) —
    // не сравниваем мусорные нули, а уходим на CPU
    cudaError_t launchErr = cudaGetLastError();
    if (launchErr != cudaSuccess) {
        std::cerr << "CUDA kernel error: " << cudaGetErrorString(launchErr)
                  << ". Switching to CPU fallback." << std::endl;
        cpu_fallback = true;
        ComputeHash160BatchCpu(pubkeys_x, pubkeys_y, num_keys, hash160s);
        return;
    }

    cudaError_t copyErr = cudaMemcpy(hash160s, d_hash160s, num_keys * 20, cudaMemcpyDeviceToHost);
    if (copyErr != cudaSuccess) {
        std::cerr << "CUDA memcpy error: " << cudaGetErrorString(copyErr)
                  << ". Switching to CPU fallback." << std::endl;
        cpu_fallback = true;
        ComputeHash160BatchCpu(pubkeys_x, pubkeys_y, num_keys, hash160s);
        return;
    }
}

CUDABatchProcessor g_cuda_batch;