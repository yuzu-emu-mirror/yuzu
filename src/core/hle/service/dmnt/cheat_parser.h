// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>
#include <vector>

namespace Service::DMNT {
struct CheatEntry;

class CheatParser final {
public:
    CheatParser();
    ~CheatParser();

    std::vector<CheatEntry> Parse(std::string_view data) const;

private:
    std::string_view ExtractName(std::size_t& out_name_size, std::string_view data,
                                 std::size_t start_index, char match) const;
};

} // namespace Service::DMNT
