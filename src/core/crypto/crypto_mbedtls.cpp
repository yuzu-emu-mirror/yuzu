// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef YUZU_USE_MBEDTLS_CRYPTO

#include <mbedtls/bignum.h>
#include <mbedtls/cmac.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha256.h>

#include "common/assert.h"
#include "core/crypto/crypto.h"

bool CalculateHMACSHA256(u8* out, const u8* key, std::size_t key_length, const u8* data,
                         std::size_t data_length) {
    mbedtls_md_context_t context;
    mbedtls_md_init(&context);

    if (mbedtls_md_setup(&context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) ||
        mbedtls_md_hmac_starts(&context, key, key_length) ||
        mbedtls_md_hmac_update(&context, data, data_length) ||
        mbedtls_md_hmac_finish(&context, out)) {
        mbedtls_md_free(&context);
        return false;
    }

    mbedtls_md_free(&context);
    return true;
}

void CalculateMD5(const u8* data, std::size_t data_length, u8* hash) {
    mbedtls_md5_ret(data, data_length, hash);
}

void CalculateSHA256(const u8* data, std::size_t data_length, u8* hash) {
    mbedtls_sha256_ret(data, data_length, hash, 0);
}

void CalculateCMAC(const u8* source, size_t size, const u8* key, u8* cmac) {
    mbedtls_cipher_cmac(mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB), key, 128, source,
                        size, cmac);
}

void CalculateModExp(const u8* d, std::size_t d_length, const u8* n, std::size_t n_length,
                     const u8* s, std::size_t s_length, u8* m, std::size_t m_length) {
    mbedtls_mpi D; // RSA Private Exponent
    mbedtls_mpi N; // RSA Modulus
    mbedtls_mpi S; // Input
    mbedtls_mpi M; // Output

    mbedtls_mpi_init(&D);
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&S);
    mbedtls_mpi_init(&M);

    mbedtls_mpi_read_binary(&D, d, d_length);
    mbedtls_mpi_read_binary(&N, n, n_length);
    mbedtls_mpi_read_binary(&S, s, s_length);

    mbedtls_mpi_exp_mod(&M, &S, &D, &N, nullptr);
    mbedtls_mpi_write_binary(&M, m, m_length);
}

void GenerateRandomBytesWithSeed(u8* out, std::size_t out_length, const u8* seed,
                                 std::size_t seed_length) {
    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_ctr_drbg_init(&ctr_drbg);
    ASSERT(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, seed, seed_length));
    ASSERT(mbedtls_ctr_drbg_random(&ctr_drbg, out, out_length) == 0);

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

#endif
