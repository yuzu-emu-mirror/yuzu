// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <unordered_map>
#include <vector>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/memory_manager.h"

namespace Tegra {
namespace Engines {

class Maxwell3D final {
public:
    explicit Maxwell3D(MemoryManager& memory_manager);
    ~Maxwell3D() = default;

    /// Write the value to the register identified by method.
    void WriteReg(u32 method, u32 value);

    /**
     * Handles a method call to this engine.
     * @param method Method to call
     * @param parameters Arguments to the method call
     */
    void CallMethod(u32 method, const std::vector<u32>& parameters);

    /// Register structure of the Maxwell3D engine.
    /// TODO(Subv): This structure will need to be made bigger as more registers are discovered.
    struct Regs {
        static constexpr size_t NUM_REGS = 0xE36;

        static constexpr size_t NumCBData = 16;
        static constexpr size_t NumVertexArrays = 32;
        static constexpr size_t MaxShaderProgram = 6;
        static constexpr size_t MaxShaderType = 5;

        enum class QueryMode : u32 {
            Write = 0,
            Sync = 1,
        };

        enum class ShaderProgram : u32 {
            VertexA = 0,
            VertexB = 1,
            TesselationControl = 2,
            TesselationEval = 3,
            Geometry = 4,
            Fragment = 5,
        };

        enum class ShaderType : u32 {
            Vertex = 0,
            TesselationControl = 1,
            TesselationEval = 2,
            Geometry = 3,
            Fragment = 4,
        };

        union {
            struct {
                INSERT_PADDING_WORDS(0x582);
                struct {
                    u32 code_address_high;
                    u32 code_address_low;

                    GPUVAddr CodeAddress() const {
                        return static_cast<GPUVAddr>(
                            (static_cast<GPUVAddr>(code_address_high) << 32) | code_address_low);
                    }
                } code_address;
                INSERT_PADDING_WORDS(1);
                struct {
                    u32 vertex_end_gl;
                    u32 vertex_begin_gl;
                } draw;
                INSERT_PADDING_WORDS(0x139);
                struct {
                    u32 query_address_high;
                    u32 query_address_low;
                    u32 query_sequence;
                    union {
                        u32 raw;
                        BitField<0, 2, QueryMode> mode;
                        BitField<4, 1, u32> fence;
                        BitField<12, 4, u32> unit;
                    } query_get;

                    GPUVAddr QueryAddress() const {
                        return static_cast<GPUVAddr>(
                            (static_cast<GPUVAddr>(query_address_high) << 32) | query_address_low);
                    }
                } query;

                INSERT_PADDING_WORDS(0x3C);

                struct {
                    union {
                        BitField<0, 12, u32> stride;
                        BitField<12, 1, u32> enable;
                    };
                    u32 start_high;
                    u32 start_low;
                    u32 divisor;

                    GPUVAddr StartAddress() const {
                        return static_cast<GPUVAddr>((static_cast<GPUVAddr>(start_high) << 32) |
                                                     start_low);
                    }
                } vertex_array[NumVertexArrays];

                INSERT_PADDING_WORDS(0x40);

                struct {
                    u32 limit_high;
                    u32 limit_low;

                    GPUVAddr LimitAddress() const {
                        return static_cast<GPUVAddr>((static_cast<GPUVAddr>(limit_high) << 32) |
                                                     limit_low);
                    }
                } vertex_array_limit[NumVertexArrays];

                struct {
                    union {
                        BitField<0, 1, u32> enable;
                        BitField<4, 4, ShaderProgram> program;
                    };
                    u32 start_id;
                    INSERT_PADDING_WORDS(1);
                    u32 gpr_alloc;
                    ShaderType type;
                    INSERT_PADDING_WORDS(9);
                } shader_config[MaxShaderProgram];

                INSERT_PADDING_WORDS(0x8C);

                struct {
                    u32 cb_size;
                    u32 cb_address_high;
                    u32 cb_address_low;
                    u32 cb_pos;
                    u32 cb_data[NumCBData];
                } const_buffer;

                INSERT_PADDING_WORDS(0x74);

                struct {
                    union {
                        BitField<0, 1, u32> valid;
                        BitField<4, 5, u32> index;
                    };
                    INSERT_PADDING_WORDS(7);
                } cb_bind[MaxShaderType];

                INSERT_PADDING_WORDS(0x494);

                struct {
                    u32 set_shader_call;
                    u32 set_shader_args;
                } set_shader;
                INSERT_PADDING_WORDS(0x10);
            };
            std::array<u32, NUM_REGS> reg_array;
        };
    } regs{};

    static_assert(sizeof(Regs) == Regs::NUM_REGS * sizeof(u32), "Maxwell3D Regs has wrong size");

    struct State {
        struct ShaderInfo {
            Regs::ShaderType type;
            Regs::ShaderProgram program;
            GPUVAddr address;
            GPUVAddr cb_address;
        };

        std::array<ShaderInfo, Regs::MaxShaderProgram> shaders;
    };

    State state{};

private:
    MemoryManager& memory_manager;

    /// Handles a write to the QUERY_GET register.
    void ProcessQueryGet();

    /// Handles a write to the VERTEX_END_GL register, triggering a draw.
    void DrawArrays();

    /// Method call handlers
    void SetShader(const std::vector<u32>& parameters);

    struct MethodInfo {
        const char* name;
        u32 arguments;
        void (Maxwell3D::*handler)(const std::vector<u32>& parameters);
    };

    static const std::unordered_map<u32, MethodInfo> method_handlers;
};

#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(Maxwell3D::Regs, field_name) == position * 4,                           \
                  "Field " #field_name " has invalid position")

ASSERT_REG_POSITION(code_address, 0x582);
ASSERT_REG_POSITION(draw, 0x585);
ASSERT_REG_POSITION(query, 0x6C0);
ASSERT_REG_POSITION(vertex_array[0], 0x700);
ASSERT_REG_POSITION(vertex_array_limit[0], 0x7C0);
ASSERT_REG_POSITION(shader_config[0], 0x800);
ASSERT_REG_POSITION(const_buffer, 0x8E0);
ASSERT_REG_POSITION(set_shader, 0xE24);

#undef ASSERT_REG_POSITION

} // namespace Engines
} // namespace Tegra
