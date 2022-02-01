// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>

namespace Common {

/// A string-based key-value container supporting serializing to and deserializing from a string
class ParamPackage {
public:
    struct DataHash final {
        using is_transparent = void;

        [[nodiscard]] size_t operator()(std::string_view view) const noexcept {
            return std::hash<std::string_view>{}(view);
        }
        [[nodiscard]] size_t operator()(const std::string& str) const noexcept {
            return std::hash<std::string>{}(str);
        }
    };
    using DataType = std::unordered_map<std::string, std::string, DataHash, std::equal_to<>>;

    ParamPackage() = default;
    explicit ParamPackage(const std::string& serialized);
    ParamPackage(std::initializer_list<DataType::value_type> list);
    ParamPackage(const ParamPackage& other) = default;
    ParamPackage(ParamPackage&& other) noexcept = default;

    ParamPackage& operator=(const ParamPackage& other) = default;
    ParamPackage& operator=(ParamPackage&& other) = default;

    [[nodiscard]] std::string Serialize() const;
    [[nodiscard]] std::string Get(std::string_view key, const std::string& default_value) const;
    [[nodiscard]] int Get(std::string_view key, int default_value) const;
    [[nodiscard]] float Get(std::string_view key, float default_value) const;
    void Set(const std::string& key, std::string value);
    void Set(const std::string& key, int value);
    void Set(const std::string& key, float value);
    [[nodiscard]] bool Has(std::string_view key) const;
    void Erase(const std::string& key);
    void Clear();

private:
    DataType data;
};

} // namespace Common
