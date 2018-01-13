// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hw/aes/ccm.h"
#include "core/hw/aes/key.h"

namespace HW {
namespace AES {

std::vector<u8> EncryptSignCCM(const std::vector<u8>& pdata, const CCMNonce& nonce,
                               size_t slot_id) {
    UNIMPLEMENTED();
    return {};
}

std::vector<u8> DecryptVerifyCCM(const std::vector<u8>& cipher, const CCMNonce& nonce,
                                 size_t slot_id) {
    UNIMPLEMENTED();
    return {};
}

} // namespace AES
} // namespace HW
