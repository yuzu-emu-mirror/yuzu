// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>
#include <lz4.h>

#include "common/logging/log.h"
#include "common/swap.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/loader/nso.h"
#include "core/memory.h"

namespace Loader {

enum class RelocationType : u32 { ABS64 = 257, GLOB_DAT = 1025, JUMP_SLOT = 1026, RELATIVE = 1027 };

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
    std::array<NsoSegmentHeader, 3> segments; // Text, RoData, Data (in that order)
    u32_le bss_size;
    INSERT_PADDING_BYTES(0x1c);
    std::array<u32_le, 3> segments_compressed_size;
};
static_assert(sizeof(NsoHeader) == 0x6c, "NsoHeader has incorrect size.");

struct ModHeader {
    INSERT_PADDING_BYTES(0x4);
    u32_le offset_to_start; // Always 8
    u32_le magic;
    u32_le dynamic_offset;
    u32_le bss_start_offset;
    u32_le bss_end_offset;
    u32_le eh_frame_hdr_start_offset;
    u32_le eh_frame_hdr_end_offset;
    u32_le module_offset; // Offset to runtime-generated module object. typically equal to .bss base
};
static_assert(sizeof(ModHeader) == 0x24, "ModHeader has incorrect size.");

FileType AppLoader_NSO::IdentifyType(FileUtil::IOFile& file) {
    u32 magic = 0;
    file.Seek(0, SEEK_SET);
    if (1 != file.ReadArray<u32>(&magic, 1)) {
        return FileType::Error;
    }

    if (MakeMagic('N', 'S', 'O', '0') == magic) {
        return FileType::NSO;
    }

    return FileType::Error;
}

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

void AppLoader_NSO::WriteRelocations(const std::vector<Symbol>& symbols, VAddr load_base,
                                     u64 relocation_offset, u64 size, bool is_jump_relocation) {
    for (u64 i = 0; i < size; i += 0x18) {
        VAddr addr = load_base + relocation_offset + i;
        u64 offset = Memory::Read64(addr);
        u64 info = Memory::Read64(addr + 8);
        u64 addend_unsigned = Memory::Read64(addr + 16);
        s64 addend{};
        std::memcpy(&addend, &addend_unsigned, sizeof(u64));

        RelocationType rtype = static_cast<RelocationType>(info & 0xFFFFFFFF);
        u32 rsym = static_cast<u32>(info >> 32);
        VAddr ea = load_base + offset;

        const Symbol& symbol = symbols[rsym];

        switch (rtype) {
        case RelocationType::RELATIVE:
            if (!symbol.name.empty()) {
                exports[symbol.name] = load_base + addend;
            }
            Memory::Write64(ea, load_base + addend);
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

void AppLoader_NSO::Relocate(VAddr load_base, VAddr dynamic_section_addr) {
    std::map<u64, u64> dynamic;
    while (1) {
        u64 tag = Memory::Read64(dynamic_section_addr);
        u64 value = Memory::Read64(dynamic_section_addr + 8);
        dynamic_section_addr += 16;

        if (tag == DT_NULL) {
            break;
        }
        dynamic[tag] = value;
    }

    u64 strtabsize = dynamic[DT_STRSZ];
    std::vector<u8> strtab;
    strtab.resize(strtabsize);
    Memory::ReadBlock(load_base + dynamic[DT_STRTAB], strtab.data(), strtabsize);

    VAddr addr = load_base + dynamic[DT_SYMTAB];
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
            exports[name] = load_base + stvalue;
            symbols.emplace_back(std::move(name), load_base + stvalue);
        } else {
            symbols.emplace_back(std::move(name), 0);
        }
    }

    if (dynamic.find(DT_RELA) != dynamic.end()) {
        WriteRelocations(symbols, load_base, dynamic[DT_RELA], dynamic[DT_RELASZ], false);
    }

    if (dynamic.find(DT_JMPREL) != dynamic.end()) {
        WriteRelocations(symbols, load_base, dynamic[DT_JMPREL], dynamic[DT_PLTRELSZ], true);
    }
}

VAddr AppLoader_NSO::GetEntryPoint() const {
    // Find nnMain function, set entrypoint to that address
    const auto& search = exports.find("nnMain");
    if (search != exports.end()) {
        return search->second;
    }
    ASSERT_MSG(false, "Unable to find entrypoint");
    return {};
}

static constexpr u32 PageAlignSize(u32 size) {
    return (size + Memory::PAGE_MASK) & ~Memory::PAGE_MASK;
}

bool AppLoader_NSO::LoadNso(const std::string& path, VAddr load_base) {
    FileUtil::IOFile file(path, "rb");
    if (!file.IsOpen()) {
        return {};
    }

    // Read NSO header
    NsoHeader nso_header{};
    file.Seek(0, SEEK_SET);
    if (sizeof(NsoHeader) != file.ReadBytes(&nso_header, sizeof(NsoHeader))) {
        return {};
    }
    if (nso_header.magic != MakeMagic('N', 'S', 'O', '0')) {
        return {};
    }

    // Build program image
    Kernel::SharedPtr<Kernel::CodeSet> codeset = Kernel::CodeSet::Create("", 0);
    std::vector<u8> program_image;
    for (int i = 0; i < nso_header.segments.size(); ++i) {
        std::vector<u8> data =
            ReadSegment(file, nso_header.segments[i], nso_header.segments_compressed_size[i]);
        program_image.resize(nso_header.segments[i].location);
        program_image.insert(program_image.end(), data.begin(), data.end());
        codeset->segments[i].addr = nso_header.segments[i].location;
        codeset->segments[i].offset = nso_header.segments[i].location;
        codeset->segments[i].size = static_cast<u32>(data.size());
    }

    // Read MOD header
    ModHeader mod_header{};
    std::memcpy(&mod_header, program_image.data(), sizeof(ModHeader));
    if (mod_header.magic != MakeMagic('M', 'O', 'D', '0')) {
        return {};
    }

    // Resize program image to include .bss section and page align each section
    const u32 bss_size = mod_header.bss_end_offset - mod_header.bss_start_offset;
    codeset->code.size = PageAlignSize(codeset->code.size);
    codeset->rodata.size = PageAlignSize(codeset->rodata.size);
    codeset->data.size = PageAlignSize(codeset->data.size + bss_size);
    program_image.resize(PageAlignSize(static_cast<u32>(program_image.size()) + bss_size));

    // Load codeset for current process
    codeset->name = path;
    codeset->memory = std::make_shared<std::vector<u8>>(std::move(program_image));
    Kernel::g_current_process->LoadModule(codeset, load_base);
    Relocate(load_base, load_base + mod_header.offset_to_start + mod_header.dynamic_offset);

    return true;
}

ResultStatus AppLoader_NSO::Load() {
    if (is_loaded) {
        return ResultStatus::ErrorAlreadyLoaded;
    }
    if (!file.IsOpen()) {
        return ResultStatus::Error;
    }

    // Load and relocate "main" and "sdk" NSO
    const std::string sdkpath = filepath.substr(0, filepath.find_last_of("/\\")) + "/sdk";
    Kernel::g_current_process = Kernel::Process::Create("main");
    if (!LoadNso(filepath, 0x10000000) || !LoadNso(sdkpath, 0x20000000)) {
        return ResultStatus::ErrorInvalidFormat;
    }

    Kernel::g_current_process->svc_access_mask.set();
    Kernel::g_current_process->address_mappings = default_address_mappings;
    Kernel::g_current_process->resource_limit =
        Kernel::ResourceLimit::GetForCategory(Kernel::ResourceLimitCategory::APPLICATION);
    Kernel::g_current_process->Run(GetEntryPoint(), 48, Kernel::DEFAULT_STACK_SIZE);

    // Resolve imports
    for (const auto& import : imports) {
        const auto& search = exports.find(import.first);
        if (search != exports.end()) {
            Memory::Write64(import.second.ea, search->second + import.second.addend);
        } else {
            LOG_ERROR(Loader, "Unresolved import: %s", import.first.c_str());
        }
    }

    is_loaded = true;
    return ResultStatus::Success;
}

} // namespace Loader
