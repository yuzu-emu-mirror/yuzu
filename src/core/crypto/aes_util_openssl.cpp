#ifdef YUZU_USE_OPENSSL_CRYPTO

#include <map>
#include <vector>
#include <openssl/evp.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/crypto/aes_util.h"
#include "core/crypto/key_manager.h"

namespace Core::Crypto {
namespace {
using NintendoTweak = std::array<u8, 16>;

NintendoTweak CalculateNintendoTweak(std::size_t sector_id) {
    NintendoTweak out{};
    for (std::size_t i = 0xF; i <= 0xF; --i) {
        out[i] = sector_id & 0xFF;
        sector_id >>= 8;
    }
    return out;
}
} // Anonymous namespace

struct CipherContext {
    const EVP_CIPHER* cipher;
    EVP_CIPHER_CTX* ctx;
    std::vector<u8> key;
    std::vector<u8> iv;
};

const static std::map<Mode, decltype(&EVP_aes_128_ctr)> cipher_map = {
    {Mode::CTR, EVP_aes_128_ctr}, {Mode::ECB, EVP_aes_128_ecb}, {Mode::XTS, EVP_aes_128_xts}};

template <typename Key, std::size_t KeySize>
Crypto::AESCipher<Key, KeySize>::AESCipher(Key key, Mode mode)
    : ctx(std::make_unique<CipherContext>()) {
    ctx->ctx = EVP_CIPHER_CTX_new();
    ASSERT_MSG((ctx->ctx != NULL), "Failed to initialize OpenSSL ciphers.");
    ctx->cipher = cipher_map.at(mode)();
    ctx->key.resize(KeySize);
    std::memcpy(ctx->key.data(), key.data(), KeySize);
}

template <typename Key, std::size_t KeySize>
AESCipher<Key, KeySize>::~AESCipher() {
    EVP_CIPHER_CTX_free(ctx->ctx);
}

template <typename Key, std::size_t KeySize>
void AESCipher<Key, KeySize>::Transcode(const u8* src, std::size_t size, u8* dest, Op op) const {
    EVP_CIPHER_CTX_reset(ctx->ctx);
    EVP_CipherInit(ctx->ctx, ctx->cipher, ctx->key.data(), ctx->iv.data(),
                   op == Op::Encrypt ? 1 : 0);
    EVP_CIPHER_CTX_set_padding(ctx->ctx, 0);

    int written, last_written;
    if (EVP_CIPHER_mode(ctx->cipher) == EVP_CIPH_XTS_MODE) {
        EVP_CipherUpdate(ctx->ctx, dest, &written, src, (int)size);
        if (written != (int)size) {
            LOG_WARNING(Crypto, "Not all data was decrypted requested={:016X}, actual={:016X}.",
                        size, written);
        }
    } else {
        EVP_CipherUpdate(ctx->ctx, dest, &written, src, (int)size);
        EVP_CipherFinal(ctx->ctx, dest + written, &last_written);
        if (written + last_written != (int)size) {
            LOG_WARNING(Crypto, "Not all data was decrypted requested={:016X}, actual={:016X}.",
                        size, written + last_written);
        }
    }
}

template <typename Key, std::size_t KeySize>
void AESCipher<Key, KeySize>::XTSTranscode(const u8* src, std::size_t size, u8* dest,
                                           std::size_t sector_id, std::size_t sector_size, Op op) {
    ASSERT_MSG(size % sector_size == 0, "XTS decryption size must be a multiple of sector size.");

    for (std::size_t i = 0; i < size; i += sector_size) {
        SetIV(CalculateNintendoTweak(sector_id++));
        Transcode(src + i, sector_size, dest + i, op);
    }
}

template <typename Key, std::size_t KeySize>
void AESCipher<Key, KeySize>::SetIV(std::span<const u8> data) {
    ctx->iv.resize(data.size());
    std::memcpy(ctx->iv.data(), data.data(), data.size());
}

template class AESCipher<Key128>;
template class AESCipher<Key256>;
} // namespace Core::Crypto

#endif
