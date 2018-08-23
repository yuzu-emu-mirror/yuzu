// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace FileSys {

namespace ErrCodes {
enum {
    NotFound = 1,
    TitleNotFound = 1002,
    SdCardNotFound = 2001,
    RomFSNotFound = 2520,
};
}

// When using mingw precompiled headers, it pulls in some dark corners of the stdlib that defines
// these as macros
#ifdef ERROR_PATH_NOT_FOUND
#undef ERROR_PATH_NOT_FOUND
#endif
#ifdef ERROR_FILE_NOT_FOUND
#undef ERROR_FILE_NOT_FOUND
#endif

constexpr ResultCode ERROR_PATH_NOT_FOUND(ErrorModule::FS, ErrCodes::NotFound);

// TODO(bunnei): Replace these with correct errors for Switch OS
constexpr ResultCode ERROR_INVALID_PATH(-1);
constexpr ResultCode ERROR_UNSUPPORTED_OPEN_FLAGS(-1);
constexpr ResultCode ERROR_INVALID_OPEN_FLAGS(-1);
constexpr ResultCode ERROR_FILE_NOT_FOUND(-1);
constexpr ResultCode ERROR_UNEXPECTED_FILE_OR_DIRECTORY(-1);
constexpr ResultCode ERROR_DIRECTORY_ALREADY_EXISTS(-1);
constexpr ResultCode ERROR_FILE_ALREADY_EXISTS(-1);
constexpr ResultCode ERROR_DIRECTORY_NOT_EMPTY(-1);

} // namespace FileSys
