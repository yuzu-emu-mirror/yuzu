// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace FileSys {

namespace ErrCodes {
enum {
    RomFSNotFound = 100,
    ArchiveNotMounted = 101,
    FileNotFound = 112,
    PathNotFound = 113,
    GameCardNotInserted = 141,
    NotFound = 120,
    FileAlreadyExists = 180,
    DirectoryAlreadyExists = 185,
    AlreadyExists = 190,
    InvalidOpenFlags = 230,
    DirectoryNotEmpty = 240,
    NotAFile = 250,
    NotFormatted = 340, ///< This is used by the FS service when creating a SaveData archive
    ExeFSSectionNotFound = 567,
    CommandNotAllowed = 630,
    InvalidReadFlag = 700,
    InvalidPath = 702,
    WriteBeyondEnd = 705,
    UnsupportedOpenFlags = 760,
    IncorrectExeFSReadSize = 761,
    UnexpectedFileOrDirectory = 770,
};
}

// TODO(bunnei): Replace these with correct errors for Switch OS
constexpr ResultCode ERROR_INVALID_PATH(ResultCode(-1));
constexpr ResultCode ERROR_UNSUPPORTED_OPEN_FLAGS(ResultCode(-1));
constexpr ResultCode ERROR_INVALID_OPEN_FLAGS(ResultCode(-1));
constexpr ResultCode ERROR_FILE_NOT_FOUND(ResultCode(-1));
constexpr ResultCode ERROR_PATH_NOT_FOUND(ResultCode(-1));
constexpr ResultCode ERROR_UNEXPECTED_FILE_OR_DIRECTORY(ResultCode(-1));
constexpr ResultCode ERROR_DIRECTORY_ALREADY_EXISTS(ResultCode(-1));
constexpr ResultCode ERROR_FILE_ALREADY_EXISTS(ResultCode(-1));
constexpr ResultCode ERROR_DIRECTORY_NOT_EMPTY(ResultCode(-1));

} // namespace FileSys
