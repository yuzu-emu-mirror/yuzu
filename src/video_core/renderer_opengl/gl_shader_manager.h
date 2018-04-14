// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <tuple>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <glad/glad.h>
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_shader_gen.h"
#include "video_core/renderer_opengl/maxwell_to_gl.h"

namespace GLShader {

/// Number of OpenGL texture samplers that can be used in the fragment shader
static constexpr size_t NumTextureSamplers = 32;

using Tegra::Engines::Maxwell3D;

namespace Impl {
void SetShaderUniformBlockBindings(GLuint shader);
void SetShaderSamplerBindings(GLuint shader);
} // namespace Impl

/// Uniform structure for the Uniform Buffer Object, all vectors must be 16-byte aligned
// NOTE: Always keep a vec4 at the end. The GL spec is not clear wether the alignment at
//       the end of a uniform block is included in UNIFORM_BLOCK_DATA_SIZE or not.
//       Not following that rule will cause problems on some AMD drivers.
struct MaxwellUniformData {
    void SetFromRegs(const Maxwell3D::State::ShaderStageInfo& shader_stage);

    using ConstBuffer = std::array<GLvec4, 4>;
    alignas(16) std::array<ConstBuffer, Maxwell3D::Regs::MaxConstBuffers> const_buffers;
};
static_assert(sizeof(MaxwellUniformData) == 1024, "MaxwellUniformData structure size is incorrect");
static_assert(sizeof(MaxwellUniformData) < 16384,
              "MaxwellUniformData structure must be less than 16kb as per the OpenGL spec");

class OGLShaderStage {
public:
    OGLShaderStage() = default;

    void Create(const char* source, GLenum type) {
        OGLShader shader;
        shader.Create(source, type);
        program.Create(true, shader.handle);
        Impl::SetShaderUniformBlockBindings(program.handle);
        Impl::SetShaderSamplerBindings(program.handle);
    }
    GLuint GetHandle() const {
        return program.handle;
    }

private:
    OGLProgram program;
};

// TODO(wwylele): beautify this doc
// This is a shader cache designed for translating PICA shader to GLSL shader.
// The double cache is needed because diffent KeyConfigType, which includes a hash of the code
// region (including its leftover unused code) can generate the same GLSL code.
template <typename KeyConfigType,
          std::string (*CodeGenerator)(const ShaderSetup&, const KeyConfigType&), GLenum ShaderType>
class ShaderCache {
public:
    ShaderCache() = default;

    GLuint Get(const KeyConfigType& key, const ShaderSetup& setup) {
        auto map_it = shader_map.find(key);
        if (map_it == shader_map.end()) {
            std::string program = CodeGenerator(setup, key);

            auto [iter, new_shader] = shader_cache.emplace(program, OGLShaderStage{});
            OGLShaderStage& cached_shader = iter->second;
            if (new_shader) {
                cached_shader.Create(program.c_str(), ShaderType);
            }
            shader_map[key] = &cached_shader;
            return cached_shader.GetHandle();
        } else {
            return map_it->second->GetHandle();
        }
    }

private:
    std::unordered_map<KeyConfigType, OGLShaderStage*> shader_map;
    std::unordered_map<std::string, OGLShaderStage> shader_cache;
};

using VertexShaders = ShaderCache<MaxwellVSConfig, &GenerateVertexShader, GL_VERTEX_SHADER>;

using FragmentShaders = ShaderCache<MaxwellFSConfig, &GenerateFragmentShader, GL_FRAGMENT_SHADER>;

class ProgramManager {
public:
    ProgramManager() {
        pipeline.Create();
    }

    void UseProgrammableVertexShader(const MaxwellVSConfig& config, const ShaderSetup setup) {
        current.vs = vertex_shaders.Get(config, setup);
    }

    void UseProgrammableFragmentShader(const MaxwellFSConfig& config, const ShaderSetup setup) {
        current.fs = fragment_shaders.Get(config, setup);
    }

    void UseTrivialGeometryShader() {
        current.gs = 0;
    }

    void ApplyTo(OpenGLState& state) {
        // Workaround for AMD bug
        glUseProgramStages(pipeline.handle,
                           GL_VERTEX_SHADER_BIT | GL_GEOMETRY_SHADER_BIT | GL_FRAGMENT_SHADER_BIT,
                           0);

        glUseProgramStages(pipeline.handle, GL_VERTEX_SHADER_BIT, current.vs);
        glUseProgramStages(pipeline.handle, GL_GEOMETRY_SHADER_BIT, current.gs);
        glUseProgramStages(pipeline.handle, GL_FRAGMENT_SHADER_BIT, current.fs);
        state.draw.shader_program = 0;
        state.draw.program_pipeline = pipeline.handle;
    }

private:
    struct ShaderTuple {
        GLuint vs = 0, gs = 0, fs = 0;
        bool operator==(const ShaderTuple& rhs) const {
            return std::tie(vs, gs, fs) == std::tie(rhs.vs, rhs.gs, rhs.fs);
        }
        struct Hash {
            std::size_t operator()(const ShaderTuple& tuple) const {
                std::size_t hash = 0;
                boost::hash_combine(hash, tuple.vs);
                boost::hash_combine(hash, tuple.gs);
                boost::hash_combine(hash, tuple.fs);
                return hash;
            }
        };
    };
    ShaderTuple current;
    VertexShaders vertex_shaders;
    FragmentShaders fragment_shaders;

    std::unordered_map<ShaderTuple, OGLProgram, ShaderTuple::Hash> program_cache;
    OGLPipeline pipeline;
};

} // namespace GLShader
