// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <glad/glad.h>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/math_util.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#include "common/vector_math.h"
#include "core/core.h"
#include "core/hle/kernel/process.h"
#include "core/settings.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_opengl/gl_rasterizer.h"
#include "video_core/renderer_opengl/gl_shader_gen.h"
#include "video_core/renderer_opengl/maxwell_to_gl.h"
#include "video_core/renderer_opengl/renderer_opengl.h"

using Maxwell = Tegra::Engines::Maxwell3D::Regs;
using PixelFormat = SurfaceParams::PixelFormat;
using SurfaceType = SurfaceParams::SurfaceType;

MICROPROFILE_DEFINE(OpenGL_VAO, "OpenGL", "Vertex Array Setup", MP_RGB(128, 128, 192));
MICROPROFILE_DEFINE(OpenGL_VS, "OpenGL", "Vertex Shader Setup", MP_RGB(128, 128, 192));
MICROPROFILE_DEFINE(OpenGL_FS, "OpenGL", "Fragment Shader Setup", MP_RGB(128, 128, 192));
MICROPROFILE_DEFINE(OpenGL_Drawing, "OpenGL", "Drawing", MP_RGB(128, 128, 192));
MICROPROFILE_DEFINE(OpenGL_Blits, "OpenGL", "Blits", MP_RGB(100, 100, 255));
MICROPROFILE_DEFINE(OpenGL_CacheManagement, "OpenGL", "Cache Mgmt", MP_RGB(100, 255, 100));

RasterizerOpenGL::RasterizerOpenGL() {
    has_ARB_buffer_storage = false;
    has_ARB_direct_state_access = false;
    has_ARB_separate_shader_objects = false;
    has_ARB_vertex_attrib_binding = false;

    // Create sampler objects
    for (size_t i = 0; i < texture_samplers.size(); ++i) {
        texture_samplers[i].Create();
        state.texture_units[i].sampler = texture_samplers[i].sampler.handle;
    }

    GLint ext_num;
    glGetIntegerv(GL_NUM_EXTENSIONS, &ext_num);
    for (GLint i = 0; i < ext_num; i++) {
        std::string extension{reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i))};

        if (extension == "GL_ARB_buffer_storage") {
            has_ARB_buffer_storage = true;
        } else if (extension == "GL_ARB_direct_state_access") {
            has_ARB_direct_state_access = true;
        } else if (extension == "GL_ARB_separate_shader_objects") {
            has_ARB_separate_shader_objects = true;
        } else if (extension == "GL_ARB_vertex_attrib_binding") {
            has_ARB_vertex_attrib_binding = true;
        }
    }

    ASSERT_MSG(has_ARB_separate_shader_objects, "has_ARB_separate_shader_objects is unsupported");

    // Clipping plane 0 is always enabled for PICA fixed clip plane z <= 0
    state.clip_distance[0] = true;

    // Generate VBO, VAO and UBO
    vertex_buffer = OGLStreamBuffer::MakeBuffer(GLAD_GL_ARB_buffer_storage, GL_ARRAY_BUFFER);
    vertex_buffer->Create(VERTEX_BUFFER_SIZE, VERTEX_BUFFER_SIZE / 2);
    sw_vao.Create();
    uniform_buffer.Create();

    state.draw.vertex_array = sw_vao.handle;
    state.draw.vertex_buffer = vertex_buffer->GetHandle();
    state.draw.uniform_buffer = uniform_buffer.handle;
    state.Apply();

    // Create render framebuffer
    framebuffer.Create();

    hw_vao.Create();
    hw_vao_enabled_attributes.fill(false);

    stream_buffer = OGLStreamBuffer::MakeBuffer(has_ARB_buffer_storage, GL_ARRAY_BUFFER);
    stream_buffer->Create(STREAM_BUFFER_SIZE, STREAM_BUFFER_SIZE / 2);
    state.draw.vertex_buffer = stream_buffer->GetHandle();

    shader_program_manager = std::make_unique<GLShader::ProgramManager>();

    state.draw.shader_program = 0;
    state.draw.vertex_array = hw_vao.handle;
    state.Apply();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, stream_buffer->GetHandle());

    vs_uniform_buffer.Create();
    glBindBuffer(GL_UNIFORM_BUFFER, vs_uniform_buffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GLShader::VSUniformData), nullptr, GL_STREAM_COPY);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, vs_uniform_buffer.handle);

    accelerate_draw = AccelDraw::Disabled;

    glEnable(GL_BLEND);

    LOG_CRITICAL(Render_OpenGL, "Sync fixed function OpenGL state here!");
}

RasterizerOpenGL::~RasterizerOpenGL() {
    if (stream_buffer != nullptr) {
        state.draw.vertex_buffer = stream_buffer->GetHandle();
        state.Apply();
        stream_buffer->Release();
    }
}

void RasterizerOpenGL::AnalyzeVertexArray(bool is_indexed) {
    const auto& regs = Core::System().GetInstance().GPU().Maxwell3D().regs;

    if (is_indexed) {
        UNREACHABLE();
    }

    // TODO(bunnei): Add support for 1+ vertex arrays
    vs_input_size = regs.vertex_buffer.count * regs.vertex_array[0].stride;
}

void RasterizerOpenGL::SetupVertexArray(u8* array_ptr, GLintptr buffer_offset) {
    MICROPROFILE_SCOPE(OpenGL_VAO);
    const auto& regs = Core::System().GetInstance().GPU().Maxwell3D().regs;
    const auto& memory_manager = Core::System().GetInstance().GPU().memory_manager;

    state.draw.vertex_array = hw_vao.handle;
    state.draw.vertex_buffer = stream_buffer->GetHandle();
    state.Apply();

    // TODO(bunnei): Add support for 1+ vertex arrays
    const auto& vertex_array{regs.vertex_array[0]};
    ASSERT_MSG(vertex_array.enable, "vertex array 0 is disabled?");
    ASSERT_MSG(!vertex_array.divisor, "vertex array 0 divisor is unimplemented!");
    for (unsigned index = 1; index < Maxwell::NumVertexArrays; ++index) {
        ASSERT_MSG(!regs.vertex_array[index].enable, "vertex array %d is unimplemented!", index);
    }

    // Use the vertex array as-is, assumes that the data is formatted correctly for OpenGL.
    // Enables the first 16 vertex attributes always, as we don't know which ones are actually used
    // until shader time. Note, Tegra technically supports 32, but we're cappinig this to 16 for now
    // to avoid OpenGL errors.
    for (unsigned index = 0; index < 16; ++index) {
        auto& attrib = regs.vertex_attrib_format[index];
        glVertexAttribPointer(index, attrib.ComponentCount(), MaxwellToGL::VertexType(attrib),
                              attrib.IsNormalized() ? GL_TRUE : GL_FALSE, vertex_array.stride,
                              reinterpret_cast<GLvoid*>(buffer_offset + attrib.offset));
        glEnableVertexAttribArray(index);
        hw_vao_enabled_attributes[index] = true;
    }

    // Copy vertex array data
    const u32 data_size{vertex_array.stride * regs.vertex_buffer.count};
    const VAddr data_addr{memory_manager->PhysicalToVirtualAddress(vertex_array.StartAddress())};
    res_cache.FlushRegion(data_addr, data_size, nullptr);
    Memory::ReadBlock(data_addr, array_ptr, data_size);

    array_ptr += data_size;
    buffer_offset += data_size;
}

void RasterizerOpenGL::SetupVertexShader(GLShader::VSUniformData* ub_ptr, GLintptr buffer_offset) {
    MICROPROFILE_SCOPE(OpenGL_VS);
    UNREACHABLE();
}

void RasterizerOpenGL::SetupFragmentShader(GLShader::FSUniformData* ub_ptr,
                                           GLintptr buffer_offset) {
    MICROPROFILE_SCOPE(OpenGL_FS);
    UNREACHABLE();
}

bool RasterizerOpenGL::AccelerateDrawBatch(bool is_indexed) {
    accelerate_draw = is_indexed ? AccelDraw::Indexed : AccelDraw::Arrays;
    DrawArrays();
    return true;
}

void RasterizerOpenGL::DrawArrays() {
    if (accelerate_draw == AccelDraw::Disabled)
        return;

    MICROPROFILE_SCOPE(OpenGL_Drawing);
    const auto& regs = Core::System().GetInstance().GPU().Maxwell3D().regs;

    // TODO(bunnei): Implement these
    const bool has_stencil = false;
    const bool using_color_fb = true;
    const bool using_depth_fb = false;
    const MathUtil::Rectangle<s32> viewport_rect{regs.viewport[0].GetRect()};

    const bool write_color_fb =
        state.color_mask.red_enabled == GL_TRUE || state.color_mask.green_enabled == GL_TRUE ||
        state.color_mask.blue_enabled == GL_TRUE || state.color_mask.alpha_enabled == GL_TRUE;

    const bool write_depth_fb =
        (state.depth.test_enabled && state.depth.write_mask == GL_TRUE) ||
        (has_stencil && state.stencil.test_enabled && state.stencil.write_mask != 0);

    Surface color_surface;
    Surface depth_surface;
    MathUtil::Rectangle<u32> surfaces_rect;
    std::tie(color_surface, depth_surface, surfaces_rect) =
        res_cache.GetFramebufferSurfaces(using_color_fb, using_depth_fb, viewport_rect);

    const u16 res_scale = color_surface != nullptr
                              ? color_surface->res_scale
                              : (depth_surface == nullptr ? 1u : depth_surface->res_scale);

    MathUtil::Rectangle<u32> draw_rect{
        static_cast<u32>(MathUtil::Clamp<s32>(static_cast<s32>(surfaces_rect.left) +
                                                  viewport_rect.left * res_scale,
                                              surfaces_rect.left, surfaces_rect.right)), // Left
        static_cast<u32>(MathUtil::Clamp<s32>(static_cast<s32>(surfaces_rect.bottom) +
                                                  viewport_rect.top * res_scale,
                                              surfaces_rect.bottom, surfaces_rect.top)), // Top
        static_cast<u32>(MathUtil::Clamp<s32>(static_cast<s32>(surfaces_rect.left) +
                                                  viewport_rect.right * res_scale,
                                              surfaces_rect.left, surfaces_rect.right)), // Right
        static_cast<u32>(MathUtil::Clamp<s32>(static_cast<s32>(surfaces_rect.bottom) +
                                                  viewport_rect.bottom * res_scale,
                                              surfaces_rect.bottom, surfaces_rect.top))}; // Bottom

    // Bind the framebuffer surfaces
    BindFramebufferSurfaces(color_surface, depth_surface, has_stencil);

    // Sync the viewport
    SyncViewport(surfaces_rect, res_scale);

    // TODO(bunnei): Sync framebuffer_scale uniform here
    // TODO(bunnei): Sync scissorbox uniform(s) here

    // Sync and bind the texture surfaces
    BindTextures();

    // Viewport can have negative offsets or larger dimensions than our framebuffer sub-rect. Enable
    // scissor test to prevent drawing outside of the framebuffer region
    state.scissor.enabled = true;
    state.scissor.x = draw_rect.left;
    state.scissor.y = draw_rect.bottom;
    state.scissor.width = draw_rect.GetWidth();
    state.scissor.height = draw_rect.GetHeight();
    state.Apply();

    // Draw the vertex batch
    const bool is_indexed = accelerate_draw == AccelDraw::Indexed;
    AnalyzeVertexArray(is_indexed);
    state.draw.vertex_buffer = stream_buffer->GetHandle();
    state.Apply();

    size_t buffer_size = static_cast<size_t>(vs_input_size);
    if (is_indexed) {
        UNREACHABLE();
    }
    buffer_size += sizeof(GLShader::VSUniformData);

    size_t ptr_pos = 0;
    u8* buffer_ptr;
    GLintptr buffer_offset;
    std::tie(buffer_ptr, buffer_offset) =
        stream_buffer->Map(static_cast<GLsizeiptr>(buffer_size), 4);

    SetupVertexArray(buffer_ptr, buffer_offset);
    ptr_pos += vs_input_size;

    GLintptr index_buffer_offset = 0;
    if (is_indexed) {
        UNREACHABLE();
    }

    SetupVertexShader(reinterpret_cast<GLShader::VSUniformData*>(&buffer_ptr[ptr_pos]),
                      buffer_offset + static_cast<GLintptr>(ptr_pos));
    const GLintptr vs_ubo_offset = buffer_offset + static_cast<GLintptr>(ptr_pos);
    ptr_pos += sizeof(GLShader::VSUniformData);

    stream_buffer->Unmap();

    const auto copy_buffer = [&](GLuint handle, GLintptr offset, GLsizeiptr size) {
        if (has_ARB_direct_state_access) {
            glCopyNamedBufferSubData(stream_buffer->GetHandle(), handle, offset, 0, size);
        } else {
            glBindBuffer(GL_COPY_WRITE_BUFFER, handle);
            glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, offset, 0, size);
        }
    };

    copy_buffer(vs_uniform_buffer.handle, vs_ubo_offset, sizeof(GLShader::VSUniformData));

    shader_program_manager->ApplyTo(state);
    state.Apply();

    if (is_indexed) {
        UNREACHABLE();
    } else {
        glDrawArrays(MaxwellToGL::PrimitiveTopology(regs.draw.topology), 0,
                     regs.vertex_buffer.count);
    }

    // Disable scissor test
    state.scissor.enabled = false;

    accelerate_draw = AccelDraw::Disabled;

    // Unbind textures for potential future use as framebuffer attachments
    for (auto& texture_unit : state.texture_units) {
        texture_unit.texture_2d = 0;
    }
    state.Apply();

    // Mark framebuffer surfaces as dirty
    MathUtil::Rectangle<u32> draw_rect_unscaled{
        draw_rect.left / res_scale, draw_rect.top / res_scale, draw_rect.right / res_scale,
        draw_rect.bottom / res_scale};

    if (color_surface != nullptr && write_color_fb) {
        auto interval = color_surface->GetSubRectInterval(draw_rect_unscaled);
        res_cache.InvalidateRegion(boost::icl::first(interval), boost::icl::length(interval),
                                   color_surface);
    }
    if (depth_surface != nullptr && write_depth_fb) {
        auto interval = depth_surface->GetSubRectInterval(draw_rect_unscaled);
        res_cache.InvalidateRegion(boost::icl::first(interval), boost::icl::length(interval),
                                   depth_surface);
    }
}

void RasterizerOpenGL::BindTextures() {
    using Regs = Tegra::Engines::Maxwell3D::Regs;
    auto maxwell3d = Core::System::GetInstance().GPU().Get3DEngine();

    // Each Maxwell shader stage can have an arbitrary number of textures, but we're limited to a
    // certain number in OpenGL. We try to only use the minimum amount of host textures by not
    // keeping a 1:1 relation between guest texture ids and host texture ids, ie, guest texture id 8
    // can be host texture id 0 if it's the only texture used in the guest shader program.
    u32 host_texture_index = 0;
    for (u32 stage = 0; stage < Regs::MaxShaderStage; ++stage) {
        ASSERT(host_texture_index < texture_samplers.size());
        const auto textures = maxwell3d.GetStageTextures(static_cast<Regs::ShaderStage>(stage));
        for (unsigned texture_index = 0; texture_index < textures.size(); ++texture_index) {
            const auto& texture = textures[texture_index];

            if (texture.enabled) {
                texture_samplers[host_texture_index].SyncWithConfig(texture.tsc);
                Surface surface = res_cache.GetTextureSurface(texture);
                if (surface != nullptr) {
                    state.texture_units[host_texture_index].texture_2d = surface->texture.handle;
                } else {
                    // Can occur when texture addr is null or its memory is unmapped/invalid
                    state.texture_units[texture_index].texture_2d = 0;
                }

                ++host_texture_index;
            } else {
                state.texture_units[texture_index].texture_2d = 0;
            }
        }
    }
}

void RasterizerOpenGL::NotifyMaxwellRegisterChanged(u32 id) {}

void RasterizerOpenGL::FlushAll() {
    MICROPROFILE_SCOPE(OpenGL_CacheManagement);
    res_cache.FlushAll();
}

void RasterizerOpenGL::FlushRegion(VAddr addr, u64 size) {
    MICROPROFILE_SCOPE(OpenGL_CacheManagement);
    res_cache.FlushRegion(addr, size);
}

void RasterizerOpenGL::InvalidateRegion(VAddr addr, u64 size) {
    MICROPROFILE_SCOPE(OpenGL_CacheManagement);
    res_cache.InvalidateRegion(addr, size, nullptr);
}

void RasterizerOpenGL::FlushAndInvalidateRegion(VAddr addr, u64 size) {
    MICROPROFILE_SCOPE(OpenGL_CacheManagement);
    res_cache.FlushRegion(addr, size);
    res_cache.InvalidateRegion(addr, size, nullptr);
}

bool RasterizerOpenGL::AccelerateDisplayTransfer(const void* config) {
    MICROPROFILE_SCOPE(OpenGL_Blits);
    UNREACHABLE();
    return true;
}

bool RasterizerOpenGL::AccelerateTextureCopy(const void* config) {
    UNREACHABLE();
    return true;
}

bool RasterizerOpenGL::AccelerateFill(const void* config) {
    UNREACHABLE();
    return true;
}

bool RasterizerOpenGL::AccelerateDisplay(const Tegra::FramebufferConfig& framebuffer,
                                         VAddr framebuffer_addr, u32 pixel_stride,
                                         ScreenInfo& screen_info) {
    if (framebuffer_addr == 0) {
        return false;
    }
    MICROPROFILE_SCOPE(OpenGL_CacheManagement);

    SurfaceParams src_params;
    src_params.addr = framebuffer_addr;
    src_params.width = std::min(framebuffer.width, pixel_stride);
    src_params.height = framebuffer.height;
    src_params.stride = pixel_stride;
    src_params.is_tiled = false;
    src_params.pixel_format =
        SurfaceParams::PixelFormatFromGPUPixelFormat(framebuffer.pixel_format);
    src_params.UpdateParams();

    MathUtil::Rectangle<u32> src_rect;
    Surface src_surface;
    std::tie(src_surface, src_rect) =
        res_cache.GetSurfaceSubRect(src_params, ScaleMatch::Ignore, true);

    if (src_surface == nullptr) {
        return false;
    }

    u32 scaled_width = src_surface->GetScaledWidth();
    u32 scaled_height = src_surface->GetScaledHeight();

    screen_info.display_texcoords = MathUtil::Rectangle<float>(
        (float)src_rect.bottom / (float)scaled_height, (float)src_rect.left / (float)scaled_width,
        (float)src_rect.top / (float)scaled_height, (float)src_rect.right / (float)scaled_width);

    screen_info.display_texture = src_surface->texture.handle;

    return true;
}

void RasterizerOpenGL::SamplerInfo::Create() {
    sampler.Create();
    mag_filter = min_filter = Tegra::Texture::TextureFilter::Linear;
    wrap_u = wrap_v = Tegra::Texture::WrapMode::Wrap;
    border_color_r = border_color_g = border_color_b = border_color_a = 0;

    // default is GL_LINEAR_MIPMAP_LINEAR
    glSamplerParameteri(sampler.handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // Other attributes have correct defaults
}

void RasterizerOpenGL::SamplerInfo::SyncWithConfig(const Tegra::Texture::TSCEntry& config) {
    GLuint s = sampler.handle;

    if (mag_filter != config.mag_filter) {
        mag_filter = config.mag_filter;
        glSamplerParameteri(s, GL_TEXTURE_MAG_FILTER, MaxwellToGL::TextureFilterMode(mag_filter));
    }
    if (min_filter != config.min_filter) {
        min_filter = config.min_filter;
        glSamplerParameteri(s, GL_TEXTURE_MIN_FILTER, MaxwellToGL::TextureFilterMode(min_filter));
    }

    if (wrap_u != config.wrap_u) {
        wrap_u = config.wrap_u;
        glSamplerParameteri(s, GL_TEXTURE_WRAP_S, MaxwellToGL::WrapMode(wrap_u));
    }
    if (wrap_v != config.wrap_v) {
        wrap_v = config.wrap_v;
        glSamplerParameteri(s, GL_TEXTURE_WRAP_T, MaxwellToGL::WrapMode(wrap_v));
    }

    if (wrap_u == Tegra::Texture::WrapMode::Border || wrap_v == Tegra::Texture::WrapMode::Border) {
        // TODO(Subv): Implement border color
        ASSERT(false);
    }
}

void RasterizerOpenGL::BindFramebufferSurfaces(const Surface& color_surface,
                                               const Surface& depth_surface, bool has_stencil) {
    state.draw.draw_framebuffer = framebuffer.handle;
    state.Apply();

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           color_surface != nullptr ? color_surface->texture.handle : 0, 0);
    if (depth_surface != nullptr) {
        if (has_stencil) {
            // attach both depth and stencil
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                   depth_surface->texture.handle, 0);
        } else {
            // attach depth
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                   depth_surface->texture.handle, 0);
            // clear stencil attachment
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
        }
    } else {
        // clear both depth and stencil attachment
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0,
                               0);
    }
}

void RasterizerOpenGL::SyncViewport(const MathUtil::Rectangle<u32>& surfaces_rect, u16 res_scale) {
    const auto& regs = Core::System().GetInstance().GPU().Maxwell3D().regs;
    const MathUtil::Rectangle<s32> viewport_rect{regs.viewport[0].GetRect()};

    state.viewport.x = static_cast<GLint>(surfaces_rect.left) + viewport_rect.left * res_scale;
    state.viewport.y = static_cast<GLint>(surfaces_rect.bottom) + viewport_rect.bottom * res_scale;
    state.viewport.width = static_cast<GLsizei>(viewport_rect.GetWidth() * res_scale);
    state.viewport.height = static_cast<GLsizei>(viewport_rect.GetHeight() * res_scale);
}

void RasterizerOpenGL::SyncClipEnabled() {
    UNREACHABLE();
}

void RasterizerOpenGL::SyncClipCoef() {
    UNREACHABLE();
}

void RasterizerOpenGL::SyncCullMode() {
    UNREACHABLE();
}

void RasterizerOpenGL::SyncDepthScale() {
    UNREACHABLE();
}

void RasterizerOpenGL::SyncDepthOffset() {
    UNREACHABLE();
}

void RasterizerOpenGL::SyncBlendEnabled() {
    UNREACHABLE();
}

void RasterizerOpenGL::SyncBlendFuncs() {
    UNREACHABLE();
}

void RasterizerOpenGL::SyncBlendColor() {
    UNREACHABLE();
}
