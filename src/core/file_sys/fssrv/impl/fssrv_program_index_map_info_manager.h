// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <optional>
#include "common/common_funcs.h"
#include "common/intrusive_list.h"
#include "core/file_sys/errors.h"
#include "core/file_sys/fs_program_index_map_info.h"
#include "core/hle/result.h"

namespace FileSys::FsSrv::Impl {

struct ProgramIndexMapInfoEntry : public ProgramIndexMapInfo,
                                  public Common::IntrusiveListBaseNode<ProgramIndexMapInfoEntry> {};

class ProgramIndexMapInfoManager {
    YUZU_NON_COPYABLE(ProgramIndexMapInfoManager);
    YUZU_NON_MOVEABLE(ProgramIndexMapInfoManager);

private:
    using ProgramIndexMapInfoList =
        Common::IntrusiveListBaseTraits<ProgramIndexMapInfoEntry>::ListType;

private:
    ProgramIndexMapInfoList m_list;
    mutable std::mutex m_mutex;

public:
    ProgramIndexMapInfoManager() : m_list(), m_mutex() {}

    void Clear() {
        // Acquire exclusive access to the map
        std::scoped_lock lk(m_mutex);

        // Actually clear
        this->ClearImpl();
    }

    size_t GetProgramCount() const {
        // Acquire exclusive access to the map
        std::scoped_lock lk(m_mutex);

        // Get the size
        return m_list.size();
    }

    std::optional<ProgramIndexMapInfo> Get(const u64& program_id) const {
        // Acquire exclusive access to the map
        std::scoped_lock lk(m_mutex);

        // Get the entry from the map
        return this->GetImpl(
            [&](const ProgramIndexMapInfoEntry& entry) { return entry.program_id == program_id; });
    }

    u64 GetProgramId(const u64& program_id, u8 program_index) const {
        // Acquire exclusive access to the map
        std::scoped_lock lk(m_mutex);

        // Get the program info for the desired program id
        const auto base_info = this->GetImpl(
            [&](const ProgramIndexMapInfoEntry& entry) { return entry.program_id == program_id; });

        // Check that an entry exists for the program id
        if (!base_info.has_value()) {
            return {};
        }

        // Get a program info which matches the same base program with the desired index
        const auto target_info = this->GetImpl([&](const ProgramIndexMapInfoEntry& entry) {
            return entry.base_program_id == base_info->base_program_id &&
                   entry.program_index == program_index;
        });

        // Return the desired program id
        if (target_info.has_value()) {
            return target_info->program_id;
        } else {
            return {};
        }
    }

    Result Reset(const ProgramIndexMapInfo* infos, int count) {
        // Acquire exclusive access to the map
        std::scoped_lock lk(m_mutex);

        // Clear the map, and ensure we remain clear if we fail after this point
        this->ClearImpl();
        ON_RESULT_FAILURE {
            this->ClearImpl();
        };

        // Add each info to the list
        for (int i = 0; i < count; ++i) {
            // Allocate new entry
            auto* entry = new ProgramIndexMapInfoEntry;
            R_UNLESS(entry != nullptr, ResultAllocationMemoryFailedNew);

            // Copy over the info
            entry->program_id = infos[i].program_id;
            entry->base_program_id = infos[i].base_program_id;
            entry->program_index = infos[i].program_index;

            // Add to the list
            m_list.push_back(*entry);
        }

        // We successfully imported the map
        R_SUCCEED();
    }

private:
    void ClearImpl() {
        // Delete all entries
        while (!m_list.empty()) {
            // Get the first entry
            ProgramIndexMapInfoEntry* front = std::addressof(*m_list.begin());

            // Erase it from the list
            m_list.erase(m_list.iterator_to(*front));

            // Delete the entry
            delete front;
        }
    }

    template <typename F>
    std::optional<ProgramIndexMapInfo> GetImpl(F f) const {
        // Try to find an entry matching the predicate
        std::optional<ProgramIndexMapInfo> match = std::nullopt;

        for (const auto& entry : m_list) {
            // If the predicate matches, we want to return the relevant info
            if (f(entry)) {
                match.emplace();

                match->program_id = entry.program_id;
                match->base_program_id = entry.base_program_id;
                match->program_index = entry.program_index;

                break;
            }
        }

        return match;
    }
};

} // namespace FileSys::FsSrv::Impl
