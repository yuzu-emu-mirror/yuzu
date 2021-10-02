#ifdef YUZU_USE_OPENSSL_CRYPTO

#include <openssl/bn.h>
#include <openssl/cmac.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "common/assert.h"
#include "core/crypto/crypto.h"

bool CalculateHMACSHA256(u8* out, const u8* key, std::size_t key_length, const u8* data,
                         std::size_t data_length) {
    HMAC(EVP_sha256(), key, (int)key_length, data, data_length, out, NULL);
    return true;
}

void CalculateMD5(const u8* data, std::size_t data_length, u8* hash) {
    EVP_Digest(data, data_length, hash, NULL, EVP_md5(), NULL);
}

void CalculateSHA256(const u8* data, std::size_t data_length, u8* hash) {
    EVP_Digest(data, data_length, hash, NULL, EVP_sha256(), NULL);
}

void CalculateCMAC(const u8* source, size_t size, const u8* key, u8* cmac) {
    size_t outlen;
    CMAC_CTX* ctx = CMAC_CTX_new();
    CMAC_Init(ctx, key, 16, EVP_aes_128_cbc(), NULL);
    CMAC_Update(ctx, source, size);
    CMAC_Final(ctx, cmac, &outlen);
    CMAC_CTX_free(ctx);
}

void CalculateModExp(const u8* d, std::size_t d_length, const u8* n, std::size_t n_length,
                     const u8* s, std::size_t s_length, u8* m, std::size_t m_length) {
    BN_CTX* ctx = BN_CTX_new();
    BN_CTX_start(ctx);
    BIGNUM* D = BN_CTX_get(ctx);
    BIGNUM* N = BN_CTX_get(ctx);
    BIGNUM* S = BN_CTX_get(ctx);
    BIGNUM* M = BN_CTX_get(ctx);
    BN_bin2bn(d, (int)d_length, D);
    BN_bin2bn(n, (int)n_length, N);
    BN_bin2bn(s, (int)s_length, S);
    BN_mod_exp(M, S, D, N, ctx);
    BN_bn2bin(M, m);
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
}

void GenerateRandomBytesWithSeed(u8* out, std::size_t out_length, const u8* seed,
                                 std::size_t seed_length) {
    RAND_seed((const void*)seed, (int)seed_length);
    ASSERT(RAND_bytes(out, (int)out_length) == 1);
}

#endif
