#pragma once

#include <array>
#include <string>
#include <vector>

#include "common/common_types.h"

class ShaderDumper {
public:
    ShaderDumper(const std::vector<u64>& prog, std::string prefix) : program(prog) {
        this->prefix = prefix;
    }
    void dump();

private:
    std::string hashName();

    std::string prefix;
    const std::vector<u64>& program;
};
