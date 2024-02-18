// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cctype>
#include <optional>

#include "core/hle/service/dmnt/cheat_parser.h"
#include "core/hle/service/dmnt/dmnt_types.h"

namespace Service::DMNT {

CheatParser::CheatParser() {}

CheatParser::~CheatParser() = default;

std::vector<CheatEntry> CheatParser::Parse(std::string_view data) const {
    std::vector<CheatEntry> out(1);
    std::optional<u64> current_entry;

    for (std::size_t i = 0; i < data.size(); ++i) {
        if (std::isspace(data[i])) {
            continue;
        }

        if (data[i] == '{') {
            current_entry = 0;

            if (out[*current_entry].definition.num_opcodes > 0) {
                return {};
            }

            std::size_t name_size{};
            const auto name = ExtractName(name_size, data, i + 1, '}');
            if (name.empty()) {
                return {};
            }

            std::memcpy(out[*current_entry].definition.readable_name.data(), name.data(),
                        std::min<std::size_t>(out[*current_entry].definition.readable_name.size(),
                                              name.size()));
            out[*current_entry]
                .definition.readable_name[out[*current_entry].definition.readable_name.size() - 1] =
                '\0';

            i += name_size + 1;
        } else if (data[i] == '[') {
            current_entry = out.size();
            out.emplace_back();

            std::size_t name_size{};
            const auto name = ExtractName(name_size, data, i + 1, ']');
            if (name.empty()) {
                return {};
            }

            std::memcpy(out[*current_entry].definition.readable_name.data(), name.data(),
                        std::min<std::size_t>(out[*current_entry].definition.readable_name.size(),
                                              name.size()));
            out[*current_entry]
                .definition.readable_name[out[*current_entry].definition.readable_name.size() - 1] =
                '\0';

            i += name_size + 1;
        } else if (std::isxdigit(data[i])) {
            if (!current_entry || out[*current_entry].definition.num_opcodes >=
                                      out[*current_entry].definition.opcodes.size()) {
                return {};
            }

            const auto hex = std::string(data.substr(i, 8));
            if (!std::all_of(hex.begin(), hex.end(), ::isxdigit)) {
                return {};
            }

            const auto value = static_cast<u32>(std::strtoul(hex.c_str(), nullptr, 0x10));
            out[*current_entry].definition.opcodes[out[*current_entry].definition.num_opcodes++] =
                value;

            i += 8;
        } else {
            return {};
        }
    }

    out[0].enabled = out[0].definition.num_opcodes > 0;
    out[0].cheat_id = 0;

    for (u32 i = 1; i < out.size(); ++i) {
        out[i].enabled = out[i].definition.num_opcodes > 0;
        out[i].cheat_id = i;
    }

    return out;
}

std::string_view CheatParser::ExtractName(std::size_t& out_name_size, std::string_view data,
                                          std::size_t start_index, char match) const {
    auto end_index = start_index;
    while (data[end_index] != match) {
        ++end_index;
        if (end_index > data.size()) {
            return {};
        }
    }

    out_name_size = end_index - start_index;

    // Clamp name if it's too big
    if (out_name_size > sizeof(CheatDefinition::readable_name)) {
        end_index = start_index + sizeof(CheatDefinition::readable_name);
    }

    return data.substr(start_index, end_index - start_index);
}

} // namespace Service::DMNT
