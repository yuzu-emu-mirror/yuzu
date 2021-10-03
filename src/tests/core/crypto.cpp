
#include <catch2/catch.hpp>

#include <cstring>

#include "common/common_types.h"
#include "core/crypto/aes_util.h"
#include "core/crypto/crypto.h"

constexpr static u8 msg[] = "The quick brown fox jumps over the lazy dog";
constexpr static u8 key[] = "key";

TEST_CASE("MD5", "[core]") {
    constexpr u8 hash[] = "\x9e\x10\x7d\x9d\x37\x2b\xb6\x82\x6b\xd8\x1d\x35\x42\xa4\x19\xd6";
    u8 md[16];

    CalculateMD5((const u8*)msg, sizeof(msg) - 1, md);
    REQUIRE(std::memcmp(md, hash, 16) == 0);
}

TEST_CASE("SHA256", "[core]") {
    constexpr u8 hash[] = "\xd7\xa8\xfb\xb3\x07\xd7\x80\x94\x69\xca\x9a\xbc\xb0\x08\x2e\x4f\x8d"
                          "\x56\x51\xe4\x6d\x3c\xdb\x76\x2d\x02\xd0\xbf\x37\xc9\xe5\x92";
    u8 md[32];

    CalculateSHA256((const u8*)msg, sizeof(msg) - 1, md);
    REQUIRE(std::memcmp(md, hash, 32) == 0);
}

TEST_CASE("HAMCSHA256", "[core]") {
    constexpr u8 hash[] = "\xf7\xbc\x83\xf4\x30\x53\x84\x24\xb1\x32\x98\xe6\xaa\x6f\xb1\x43\xef\x4d"
                          "\x59\xa1\x49\x46\x17\x59\x97\x47\x9d\xbc\x2d\x1a\x3c\xd8";
    u8 md[32];

    CalculateHMACSHA256(md, key, sizeof(key) - 1, msg, sizeof(msg) - 1);
    REQUIRE(std::memcmp(md, hash, 32) == 0);
}

TEST_CASE("CMAC-AES", "[core]") {
    constexpr u8 cmac_key[] = "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c";
    constexpr u8 cmac_msg1[] = "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a";
    constexpr u8 cmac_msg2[] =
        "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a\xae\x2d\x8a\x57\x1e\x03"
        "\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19"
        "\x1a\x0a\x52\xef\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10";
    constexpr u8 cmac_hash1[] = "\x07\x0a\x16\xb4\x6b\x4d\x41\x44\xf7\x9b\xdd\x9d\xd0\x4a\x28\x7c";
    constexpr u8 cmac_hash2[] = "\x51\xf0\xbe\xbf\x7e\x3b\x9d\x92\xfc\x49\x74\x17\x79\x36\x3c\xfe";
    u8 cmac_md1[16], cmac_md2[16];

    CalculateCMAC(cmac_msg1, sizeof(cmac_msg1) - 1, cmac_key, cmac_md1);
    CalculateCMAC(cmac_msg2, sizeof(cmac_msg2) - 1, cmac_key, cmac_md2);
    REQUIRE(std::memcmp(cmac_md1, cmac_hash1, 16) == 0);
    REQUIRE(std::memcmp(cmac_md2, cmac_hash2, 16) == 0);
}
