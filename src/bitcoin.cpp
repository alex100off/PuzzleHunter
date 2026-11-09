#include "bitcoin.h"
#include "hash.h"
#include <secp256k1.h>
#include <stdexcept>

// ============================================================================
// Thread-local контекст secp256k1.
// Каждый поток создаёт свой собственный экземпляр,
// что позволяет избежать блокировок и даёт линейное масштабирование.
// ============================================================================
static thread_local secp256k1_context* t_ctx = nullptr;

void InitSecp256k1()
{
    if (!t_ctx) {
        t_ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    }
}

void CleanupSecp256k1()
{
    if (t_ctx) {
        secp256k1_context_destroy(t_ctx);
        t_ctx = nullptr;
    }
}

std::array<uint8_t, 33> HexPermToCompressedPubKey(const std::string& hexPerm)
{
    if (!t_ctx) throw std::runtime_error("secp256k1 not initialized");

    // ВАЖНО: дополняем нулём слева до чётной длины
    std::string padded = hexPerm;
    if (padded.size() % 2 != 0) {
        padded = "0" + padded;
    }

    uint8_t privkey[32] = {0};
    size_t offset = 32 - padded.size() / 2;
    
    for (size_t i = 0; i < padded.size(); i += 2) {
        uint8_t hi = 0, lo = 0;
        
        // Старший полубайт
        char c = padded[i];
        if (c >= '0' && c <= '9') hi = c - '0';
        else if (c >= 'a' && c <= 'f') hi = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') hi = 10 + (c - 'A');
        
        // Младший полубайт (теперь всегда есть, т.к. длина чётная)
        c = padded[i + 1];
        if (c >= '0' && c <= '9') lo = c - '0';
        else if (c >= 'a' && c <= 'f') lo = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') lo = 10 + (c - 'A');
        
        privkey[offset + i / 2] = (hi << 4) | lo;
    }


    // Проверяем, что приватный ключ валиден (0 < key < n)
    if (!secp256k1_ec_seckey_verify(t_ctx, privkey)) {
        throw std::runtime_error("Invalid private key");
    }

    // Вычисляем публичный ключ
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(t_ctx, &pubkey, privkey)) {
        throw std::runtime_error("secp256k1_ec_pubkey_create failed");
    }

    // Сериализуем в сжатом формате (33 байта: 02/03 + 32 байта X)
    std::array<uint8_t, 33> result;
    size_t outputLen = 33;
    secp256k1_ec_pubkey_serialize(
        t_ctx, result.data(), &outputLen, &pubkey, SECP256K1_EC_COMPRESSED);

    return result;
}

void PubKeyToHash160(const uint8_t* pubkey, size_t len, uint8_t out[20])
{
    // HASH160 = RIPEMD160(SHA256(data))
    hash160(pubkey, len, out);
}

std::string BytesToHex(const uint8_t* data, size_t len)
{
    static const char HEX_CHARS[] = "0123456789abcdef";
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        result.push_back(HEX_CHARS[data[i] >> 4]);
        result.push_back(HEX_CHARS[data[i] & 0x0F]);
    }
    return result;
}