// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/result.h"

namespace FileSys {

class StorageBackend;
class DirectoryBackend;

// Path string type
enum LowPathType : u32 {
    Invalid = 0,
    Empty = 1,
    Binary = 2,
    Char = 3,
    Wchar = 4,
};

enum EntryType : u8 {
    Directory = 0,
    File = 1,
};

enum class Mode : u32 {
    Read = 1,
    Write = 2,
    Append = 4,
};

class Path {
public:
    Path() : type(Invalid) {}
    Path(const char* path) : type(Char), string(path) {}
    Path(std::vector<u8> binary_data) : type(Binary), binary(std::move(binary_data)) {}
    Path(LowPathType type, u32 size, u32 pointer);

    LowPathType GetType() const {
        return type;
    }

    /**
     * Gets the string representation of the path for debugging
     * @return String representation of the path for debugging
     */
    std::string DebugStr() const;

    std::string AsString() const;
    std::u16string AsU16Str() const;
    std::vector<u8> AsBinary() const;

private:
    LowPathType type;
    std::vector<u8> binary;
    std::string string;
    std::u16string u16str;
};

} // namespace FileSys
