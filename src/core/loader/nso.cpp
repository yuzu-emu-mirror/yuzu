// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include <string>
#include <vector>
#include <lz4.h>

#include "common/logging/log.h"
#include "common/swap.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/loader/nso.h"
#include "core/memory.h"

using Kernel::CodeSet;
using Kernel::SharedPtr;

namespace Loader {

FileType AppLoader_NSO::IdentifyType(FileUtil::IOFile& file) {
    u32 magic = 0;
    file.Seek(0, SEEK_SET);
    if (1 != file.ReadArray<u32>(&magic, 1))
        return FileType::Error;

    if (MakeMagic('N', 'S', 'O', '0') == magic)
        return FileType::NSO;

    return FileType::Error;
}

struct NsoSegmentHeader {
    u32_le offset;
    u32_le location;
    u32_le size;
    u32_le alignment;
};
static_assert(sizeof(NsoSegmentHeader) == 0x10, "NsoSegmentHeader has incorrect size.");

struct NsoHeader {
    u32_le magic;
    INSERT_PADDING_BYTES(0xc);
    std::array<NsoSegmentHeader, 3> segments; // Text, Data, RoData (in that order)
    INSERT_PADDING_BYTES(0x20);
    std::array<u32_le, 3> segments_compressed_size;
};

static_assert(sizeof(NsoHeader) == 0x6c, "NsoHeader has incorrect size.");

static std::vector<u8> ReadSegment(FileUtil::IOFile& file, const NsoSegmentHeader& header,
                                   int compressed_size) {
    std::vector<u8> compressed_data;
    compressed_data.resize(compressed_size);

    file.Seek(header.offset, SEEK_SET);
    if (compressed_size != file.ReadBytes(compressed_data.data(), compressed_size)) {
        LOG_CRITICAL(Loader, "Failed to read %d NSO LZ4 compressed bytes", compressed_size);
        return {};
    }

    std::vector<u8> uncompressed_data;
    uncompressed_data.resize(header.size);
    const int bytes_uncompressed =
        LZ4_decompress_safe_partial(reinterpret_cast<const char*>(compressed_data.data()),
                                    reinterpret_cast<char*>(uncompressed_data.data()),
                                    compressed_size, header.size, header.size);

    ASSERT_MSG(bytes_uncompressed == header.size, "%d != %d", bytes_uncompressed, header.size);

    return uncompressed_data;
}

struct Symbol {
    Symbol(std::string&& name, u64 value) : name(std::move(name)), value(value) {}
    std::string name;
    u64 value;
};

struct Import {
    VAddr ea;
    s64 addend;
};

enum class RelocationType : u32 {
    ABS64 = 257,
    GLOB_DAT = 1025,
    JUMP_SLOT = 1026,
    RELATIVE = 1027
};

enum DynamicType : u32 {
    DT_NULL = 0,
    DT_PLTRELSZ = 2,
    DT_STRTAB = 5,
    DT_SYMTAB = 6,
    DT_RELA = 7,
    DT_RELASZ = 8,
    DT_STRSZ = 10,
    DT_JMPREL = 23,
};

void WriteRelocations(const std::vector<Symbol>& symbols, VAddr loadbase, u64 roff, u64 size,
                      bool is_jump_relocation, std::map<std::string, Import>& imports,
                      std::map<std::string, VAddr>& exports) {
    for (u64 i = 0; i < size; i += 0x18) {
        VAddr addr = loadbase + roff + i;
        u64 offset = Memory::Read64(addr);
        u64 info = Memory::Read64(addr + 8);
        u64 addend_unsigned = Memory::Read64(addr + 16);
        s64 addend{};
        std::memcpy(&addend, &addend_unsigned, sizeof(u64));

        RelocationType rtype = static_cast<RelocationType>(info & 0xFFFFFFFF);
        u32 rsym = static_cast<u32>(info >> 32);
        VAddr ea = loadbase + offset;

        const Symbol& symbol = symbols[rsym];

        switch (rtype) {
        case RelocationType::RELATIVE:
            if (!symbol.name.empty()) {
                exports[symbol.name] = loadbase + addend;
            }
            Memory::Write64(ea, loadbase + addend);
            break;
        case RelocationType::JUMP_SLOT:
        case RelocationType::GLOB_DAT:
            if (!symbol.value) {
                imports[symbol.name] = {ea, 0};
            } else {
                exports[symbol.name] = symbol.value;
                Memory::Write64(ea, symbol.value);
            }
            break;
        case RelocationType::ABS64:
            if (!symbol.value) {
                imports[symbol.name] = {ea, addend};
            } else {
                exports[symbol.name] = symbol.value + addend;
                Memory::Write64(ea, symbol.value + addend);
            }
            break;
        default:
            LOG_CRITICAL(Loader, "Unknown relocation type: %d", rtype);
            break;
        }
    }
}

void Relocate(VAddr loadbase, std::map<std::string, Import>& imports,
              std::map<std::string, VAddr>& exports) {
    u32 modoff = Memory::Read32(loadbase + 4);
    ASSERT_MSG(Memory::Read32(loadbase + modoff) == MakeMagic('M', 'O', 'D', '0'),
               "Expected MOD section");

    u64 dynoff = loadbase + modoff + Memory::Read32(loadbase + modoff + 4);
    std::map<u64, u64> dynamic;
    while (1) {
        u64 tag = Memory::Read64(dynoff);
        u64 value = Memory::Read64(dynoff + 8);
        dynoff += 16;

        if (tag == DT_NULL) {
            break;
        }
        dynamic[tag] = value;
    }

    u64 strtabsize = dynamic[DT_STRSZ];
    std::vector<u8> strtab;
    strtab.resize(strtabsize);
    Memory::ReadBlock(loadbase + dynamic[DT_STRTAB], strtab.data(), strtabsize);

    VAddr addr = loadbase + dynamic[DT_SYMTAB];
    std::vector<Symbol> symbols;
    while (1) {
        const u32 stname = Memory::Read32(addr);
        const u16 stshndx = Memory::Read16(addr + 6);
        const u64 stvalue = Memory::Read64(addr + 8);
        addr += 24;

        if (stname >= strtabsize) {
            break;
        }

        std::string name = reinterpret_cast<char*>(&strtab[stname]);
        if (stvalue) {
            exports[name] = loadbase + stvalue;
            symbols.emplace_back(std::move(name), loadbase + stvalue);
        } else {
            symbols.emplace_back(std::move(name), 0);
        }
    }

    if (dynamic.find(DT_RELA) != dynamic.end()) {
        WriteRelocations(symbols, loadbase, dynamic[DT_RELA], dynamic[DT_RELASZ], false, imports,
                         exports);
    }

    if (dynamic.find(DT_JMPREL) != dynamic.end()) {
        WriteRelocations(symbols, loadbase, dynamic[DT_JMPREL], dynamic[DT_PLTRELSZ], true, imports,
                         exports);
    }
}

static VAddr GetEntryPoint(const std::map<std::string, VAddr>& exports) {
    // Find nnMain function, set entrypoint to that address
    const auto& search = exports.find("nnMain");
    if (search != exports.end()) {
        return search->second;
    }
    return {};
}

static SharedPtr<CodeSet> LoadModule(const std::string& filepath, VAddr loadbase,
                                     std::map<std::string, Import>& imports,
                                     std::map<std::string, VAddr>& exports) {
    FileUtil::IOFile file(filepath, "rb");

    if (!file.IsOpen())
        return {};

    NsoHeader header{};
    file.Seek(0, SEEK_SET);
    if (sizeof(NsoHeader) != file.ReadBytes(&header, sizeof(NsoHeader)))
        return {};

    // Build program image
    SharedPtr<CodeSet> codeset = CodeSet::Create("", 0);
    std::vector<u8> program_image;
    for (int i = 0; i < header.segments.size(); ++i) {
        std::vector<u8> data =
            ReadSegment(file, header.segments[i], header.segments_compressed_size[i]);
        program_image.resize(header.segments[i].location);
        program_image.insert(program_image.end(), data.begin(), data.end());
        codeset->segments[i].addr = header.segments[i].location;
        codeset->segments[i].offset = header.segments[i].location;
        codeset->segments[i].size = (data.size() + Memory::PAGE_MASK) & ~Memory::PAGE_MASK;
    }
    program_image.resize((program_image.size() + Memory::PAGE_MASK) & ~Memory::PAGE_MASK);

    codeset->name = filepath;
    codeset->entrypoint = 0; // Set after relocation
    codeset->memory = std::make_shared<std::vector<u8>>(std::move(program_image));

    return codeset;
}

ResultStatus AppLoader_NSO::Load() {
    if (is_loaded)
        return ResultStatus::ErrorAlreadyLoaded;

    if (!file.IsOpen())
        return ResultStatus::Error;

    static constexpr VAddr loadbase = 0x7100000000;
    std::map<std::string, Import> imports;
    std::map<std::string, VAddr> exports;

    // Load and relocate "main" NSO
    auto codeset = LoadModule(filepath, loadbase, imports, exports);
    Kernel::g_current_process = Kernel::Process::Create(codeset);
    Kernel::g_current_process->svc_access_mask.set();
    Kernel::g_current_process->address_mappings = default_address_mappings;
    Kernel::g_current_process->resource_limit =
        Kernel::ResourceLimit::GetForCategory(Kernel::ResourceLimitCategory::APPLICATION);
    Kernel::g_current_process->LoadModule(codeset, loadbase);
    Relocate(loadbase, imports, exports);
    codeset->entrypoint = GetEntryPoint(exports);
    Kernel::g_current_process->Run(48, Kernel::DEFAULT_STACK_SIZE);

    // Load and relocate "sdk" NSO
    static constexpr VAddr sdkbase = 0x7200000000;
    const std::string sdkpath = filepath.substr(0, filepath.find_last_of("/\\")) + "/sdk";
    auto sdk_codeset = LoadModule(sdkpath, sdkbase, imports, exports);
    Kernel::g_current_process->LoadModule(sdk_codeset, sdkbase);
    Relocate(sdkbase, imports, exports);

    // Resolve imports
    for (const auto& import : imports) {
        const auto& search = exports.find(import.first);
        if (search != exports.end()) {
            Memory::Write64(import.second.ea, search->second + import.second.addend);
        } else {
            LOG_CRITICAL(Loader, "Unresolved import: %s", import.first.c_str());
        }
    }

    is_loaded = true;
    return ResultStatus::Success;
}

} // namespace Loader
