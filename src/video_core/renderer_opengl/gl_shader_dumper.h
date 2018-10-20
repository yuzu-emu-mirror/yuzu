#pragma once

#include <array>
#include <string>
#include <vector>

#include "common/common_types.h"
#include "common/hash.h"

class ShaderDumper {
public:
    ShaderDumper(const std::vector<u64>& prog, std::string prefix) : program(prog) {
        this->hash = GenerateHash(program);
        this->prefix = prefix;
    }
    void dump();
    void dumpText(const std::string out);

    static bool IsProgramMarked(u64 hash);

    static u64 GenerateHash(const std::vector<u64>& program) {
        return Common::ComputeHash64(program.data(), sizeof(u64) * program.size());
    }

private:
    std::string hashName();



    u64 hash;
    std::string prefix;
    const std::vector<u64>& program;
};
