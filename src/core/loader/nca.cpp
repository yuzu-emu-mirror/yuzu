// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>

#include "common/common_funcs.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/loader/nro.h"
#include "core/memory.h"
#include "nca.h"

#include "../../../externals/cryptopp/cryptopp/aes.h"
using CryptoPP::AES;

namespace Loader {

enum NcaContentType {
    NCT_PROGRAM = 0,
    NCT_META = 1,
    NCT_CONTROL = 2,
    NCT_MANUAL = 3,
    NCT_DATA = 4
};

struct NcaHeader {
    u8 rsa_signature_1[0x100];
    u8 rsa_signature_2[0x100];
    u32 magic;
    u8 is_system;
    u8 content_type;
    u8 crypto_type;
    u8 key_index;
    u64 size;
    u64 title_id;
    INSERT_PADDING_BYTES(0x4);
    u32 sdk_version;
    u8 crypto_type_2;
    INSERT_PADDING_BYTES(0x9);
    u128 rights_id;
    u128 section_tables[0x4];
    u256 hash_tables[0x4];
    u128 key_area[0x4];
    INSERT_PADDING_BYTES(0xC0);
    INSERT_PADDING_BYTES(0x800);
};
static_assert(sizeof(NcaHeader) == 0xC00, "NcaHeader has incorrect size.");

///**
// * Adapted from hactool/aes.c, void aes_xts_decrypt(aes_ctx_t *ctx, void *dst,
// *      const void *src, size_t l, size_t sector, size_t sector_size)
// * and from various other functions in aes.c (get_tweak, aes_setiv, aes_decrypt)
// *
// * Copyright (c) 2018, SciresM
// *
// * Permission to use, copy, modify, and/or distribute this software for any
// * purpose with or without fee is hereby granted, provided that the above
// * copyright notice and this permission notice appear in all copies.
// */
// template <size_t size>
// static std::array<u8, size> AesXtsDecrypt(std::array<u8, size> in, size_t sector_size) {
//    std::array<u8, 0x10> tweak;
//    ASSERT(size % sector_size == 0);
//
//    u8 sector = 0;
//
//    for (size_t i = 0; i < size; i += sector_size) {
//
//        // Get tweak
//        size_t sector_temp = sector++;
//        for (int i = 0xF; i >= 0; --i) {
//            tweak[i] = static_cast<u8>(sector_temp);
//            sector_temp >>= 8;
//        }
//
//
//    }
//}

static bool IsValidNca(const NcaHeader& header) {
    return header.magic == Common::MakeMagic('N', 'C', 'A', '2') ||
           header.magic == Common::MakeMagic('N', 'C', 'A', '3');
}

///**
// * Adapted from hactool/nca.c, int nca_decrypt_header(nca_ctx_t *ctx)
// *
// * Copyright (c) 2018, SciresM
// *
// * Permission to use, copy, modify, and/or distribute this software for any
// * purpose with or without fee is hereby granted, provided that the above
// * copyright notice and this permission notice appear in all copies.
// */
// static NcaHeader DecryptHeader(std::array<u8, 0xC00> enc_data) {
//    // Just in case its decrypted.
//    NcaHeader header = *reinterpret_cast<NcaHeader*>(enc_data.data());
//    if (IsValidNca(header))
//        return header;
//
//    std::array<u8, 0xC00> dec = AesXtsDecrypt(enc_data, 0x200);
//
//    header = *reinterpret_cast<NcaHeader*>(dec.data());
//    return header;
//}

AppLoader_NCA::AppLoader_NCA(FileUtil::IOFile&& file, std::string filepath)
    : AppLoader(std::move(file)), filepath(std::move(filepath)) {}

FileType AppLoader_NCA::IdentifyType(FileUtil::IOFile& file, const std::string&) {

    std::array<u8, 0xC00> header_enc_array{};
    if (1 != file.ReadArray(header_enc_array.data(), 0xC00))
        return FileType::Error;

    // NcaHeader header = DecryptHeader(header_enc_array);
    // TODO(DarkLordZach): Assuming everything is decrypted. Add crypto support.
    NcaHeader header = *reinterpret_cast<NcaHeader*>(header_enc_array.data());

    if (IsValidNca(header) && header.content_type == NCT_PROGRAM)
        return FileType::NCA;

    return FileType::Error;
}

ResultStatus AppLoader_NCA::Load(Kernel::SharedPtr<Kernel::Process>& process) {
    NGLOG_CRITICAL(Loader, "UNIMPLEMENTED!");
    return ResultStatus::Error;
}

} // namespace Loader
