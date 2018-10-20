
#include "common/file_util.h"
#include "common/hash.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/renderer_opengl/gl_shader_dumper.h"

template <typename I>
std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len, '0');
    for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
        rc[i] = digits[(w >> j) & 0x0f];
    return rc;
}

std::string ShaderDumper::hashName() {
    return n2hexstr(hash);
}

bool IsSchedInstruction(u32 offset, u32 main_offset) {
    // sched instructions appear once every 4 instructions.
    static constexpr size_t SchedPeriod = 4;
    u32 absolute_offset = offset - main_offset;

    return (absolute_offset % SchedPeriod) == 0;
}

void ShaderDumper::dump() {
    FileUtil::IOFile sFile;
    std::string name = prefix + hashName() + ".bin";
    sFile.Open(name, "wb");
    u32 start_offset = 10;
    u32 offset = start_offset;
    u64 size = 0;
    while (true) { // dump until hitting not finding a valid instruction
        u64 inst = program[offset];
        if (!IsSchedInstruction(offset, start_offset)) {
            if (inst == 0) {
                break;
            }
        }
        sFile.WriteArray<u64>(&inst, 1);
        size += 8;
        offset += 1;
    }
    u64 fill = 0;
    // Align to 32 bytes for nvdisasm
    while ((size % 0x20) != 0) {
        sFile.WriteArray<u64>(&fill, 1);
        size += 8;
    }
    sFile.Close();
}

void ShaderDumper::dumpText(const std::string& s) {
    FileUtil::IOFile sFile;
    std::string name = prefix + hashName() + ".txt";
    sFile.Open(name, "w");
    sFile.WriteString(s);
    sFile.Close();
}
