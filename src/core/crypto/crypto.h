#pragma once

#include <cstdlib>

#include "common/common_types.h"

bool CalculateHMACSHA256(u8* out, const u8* key, std::size_t key_length, const u8* data,
                         std::size_t data_length);

void CalculateMD5(const u8* data, std::size_t data_length, u8* hash);

void CalculateSHA256(const u8* data, std::size_t data_length, u8* hash);

// CMAC with AES-128, key_length = 16 bytes
void CalculateCMAC(const u8* source, size_t size, const u8* key, u8* cmac);

// Calculate m = (s^d) mod n
void CalculateModExp(const u8* d, std::size_t d_length, const u8* n, std::size_t n_length,
                     const u8* s, std::size_t s_length, u8* m, std::size_t m_length);

void GenerateRandomBytesWithSeed(u8* out, std::size_t out_length, const u8* seed,
                                 std::size_t seed_length);
