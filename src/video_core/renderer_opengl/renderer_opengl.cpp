// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <glad/glad.h>
#include "common/assert.h"
#include "common/bit_field.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/hw/hw.h"
#include "core/hw/lcd.h"
#include "core/memory.h"
#include "core/settings.h"
#include "core/tracer/recorder.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/video_core.h"

static const char vertex_shader[] = R"(
#version 150 core

in vec2 vert_position;
in vec2 vert_tex_coord;
out vec2 frag_tex_coord;

// This is a truncated 3x3 matrix for 2D transformations:
// The upper-left 2x2 submatrix performs scaling/rotation/mirroring.
// The third column performs translation.
// The third row could be used for projection, which we don't need in 2D. It hence is assumed to
// implicitly be [0, 0, 1]
uniform mat3x2 modelview_matrix;

void main() {
    // Multiply input position by the rotscale part of the matrix and then manually translate by
    // the last column. This is equivalent to using a full 3x3 matrix and expanding the vector
    // to `vec3(vert_position.xy, 1.0)`
    gl_Position = vec4(mat2(modelview_matrix) * vert_position + modelview_matrix[2], 0.0, 1.0);
    frag_tex_coord = vert_tex_coord;
}
)";

static const char fragment_shader[] = R"(
#version 150 core

in vec2 frag_tex_coord;
out vec4 color;

uniform sampler2D color_texture;

void main() {
    // Swap RGBA -> ABGR so we don't have to do this on the CPU. This needs to change if we have to
    // support more framebuffer pixel formats.
    color = texture(color_texture, frag_tex_coord).abgr;
}
)";

/**
 * Vertex structure that the drawn screen rectangles are composed of.
 */
struct ScreenRectVertex {
    ScreenRectVertex(GLfloat x, GLfloat y, GLfloat u, GLfloat v) {
        position[0] = x;
        position[1] = y;
        tex_coord[0] = u;
        tex_coord[1] = v;
    }

    GLfloat position[2];
    GLfloat tex_coord[2];
};

/**
 * Defines a 1:1 pixel ortographic projection matrix with (0,0) on the top-left
 * corner and (width, height) on the lower-bottom.
 *
 * The projection part of the matrix is trivial, hence these operations are represented
 * by a 3x2 matrix.
 */
static std::array<GLfloat, 3 * 2> MakeOrthographicMatrix(const float width, const float height) {
    std::array<GLfloat, 3 * 2> matrix; // Laid out in column-major order

    // clang-format off
    matrix[0] = 2.f / width; matrix[2] = 0.f;           matrix[4] = -1.f;
    matrix[1] = 0.f;         matrix[3] = -2.f / height; matrix[5] = 1.f;
    // Last matrix row is implicitly assumed to be [0, 0, 1].
    // clang-format on

    return matrix;
}

RendererOpenGL::RendererOpenGL() = default;
RendererOpenGL::~RendererOpenGL() = default;

/// Swap buffers (render frame)
void RendererOpenGL::SwapBuffers(boost::optional<const FramebufferInfo&> framebuffer_info) {
    // Maintain the rasterizer's state as a priority
    OpenGLState prev_state = OpenGLState::GetCurState();
    state.Apply();

    if (framebuffer_info != boost::none) {
        // If framebuffer_info is provided, reload it from memory to a texture
        if (screen_info.texture.width != (GLsizei)framebuffer_info->width ||
            screen_info.texture.height != (GLsizei)framebuffer_info->height ||
            screen_info.texture.pixel_format != framebuffer_info->pixel_format) {
            // Reallocate texture if the framebuffer size has changed.
            // This is expected to not happen very often and hence should not be a
            // performance problem.
            ConfigureFramebufferTexture(screen_info.texture, *framebuffer_info);
        }
        LoadFBToScreenInfo(*framebuffer_info, screen_info);
    }

    DrawScreens();

    Core::System::GetInstance().perf_stats.EndSystemFrame();

    // Swap buffers
    render_window->PollEvents();
    render_window->SwapBuffers();

    Core::System::GetInstance().frame_limiter.DoFrameLimiting(CoreTiming::GetGlobalTimeUs());
    Core::System::GetInstance().perf_stats.BeginSystemFrame();

    prev_state.Apply();
    RefreshRasterizerSetting();
}

static inline u32 MortonInterleave128(u32 x, u32 y) {
    // 128x128 Z-Order coordinate from 2D coordinates
    static constexpr u32 xlut[] = {
        0x0000, 0x0001, 0x0002, 0x0003, 0x0008, 0x0009, 0x000a, 0x000b, 0x0040, 0x0041, 0x0042,
        0x0043, 0x0048, 0x0049, 0x004a, 0x004b, 0x0800, 0x0801, 0x0802, 0x0803, 0x0808, 0x0809,
        0x080a, 0x080b, 0x0840, 0x0841, 0x0842, 0x0843, 0x0848, 0x0849, 0x084a, 0x084b, 0x1000,
        0x1001, 0x1002, 0x1003, 0x1008, 0x1009, 0x100a, 0x100b, 0x1040, 0x1041, 0x1042, 0x1043,
        0x1048, 0x1049, 0x104a, 0x104b, 0x1800, 0x1801, 0x1802, 0x1803, 0x1808, 0x1809, 0x180a,
        0x180b, 0x1840, 0x1841, 0x1842, 0x1843, 0x1848, 0x1849, 0x184a, 0x184b, 0x2000, 0x2001,
        0x2002, 0x2003, 0x2008, 0x2009, 0x200a, 0x200b, 0x2040, 0x2041, 0x2042, 0x2043, 0x2048,
        0x2049, 0x204a, 0x204b, 0x2800, 0x2801, 0x2802, 0x2803, 0x2808, 0x2809, 0x280a, 0x280b,
        0x2840, 0x2841, 0x2842, 0x2843, 0x2848, 0x2849, 0x284a, 0x284b, 0x3000, 0x3001, 0x3002,
        0x3003, 0x3008, 0x3009, 0x300a, 0x300b, 0x3040, 0x3041, 0x3042, 0x3043, 0x3048, 0x3049,
        0x304a, 0x304b, 0x3800, 0x3801, 0x3802, 0x3803, 0x3808, 0x3809, 0x380a, 0x380b, 0x3840,
        0x3841, 0x3842, 0x3843, 0x3848, 0x3849, 0x384a, 0x384b, 0x0000, 0x0001, 0x0002, 0x0003,
        0x0008, 0x0009, 0x000a, 0x000b, 0x0040, 0x0041, 0x0042, 0x0043, 0x0048, 0x0049, 0x004a,
        0x004b, 0x0800, 0x0801, 0x0802, 0x0803, 0x0808, 0x0809, 0x080a, 0x080b, 0x0840, 0x0841,
        0x0842, 0x0843, 0x0848, 0x0849, 0x084a, 0x084b, 0x1000, 0x1001, 0x1002, 0x1003, 0x1008,
        0x1009, 0x100a, 0x100b, 0x1040, 0x1041, 0x1042, 0x1043, 0x1048, 0x1049, 0x104a, 0x104b,
        0x1800, 0x1801, 0x1802, 0x1803, 0x1808, 0x1809, 0x180a, 0x180b, 0x1840, 0x1841, 0x1842,
        0x1843, 0x1848, 0x1849, 0x184a, 0x184b, 0x2000, 0x2001, 0x2002, 0x2003, 0x2008, 0x2009,
        0x200a, 0x200b, 0x2040, 0x2041, 0x2042, 0x2043, 0x2048, 0x2049, 0x204a, 0x204b, 0x2800,
        0x2801, 0x2802, 0x2803, 0x2808, 0x2809, 0x280a, 0x280b, 0x2840, 0x2841, 0x2842, 0x2843,
        0x2848, 0x2849, 0x284a, 0x284b, 0x3000, 0x3001, 0x3002, 0x3003, 0x3008, 0x3009, 0x300a,
        0x300b, 0x3040, 0x3041, 0x3042, 0x3043, 0x3048, 0x3049, 0x304a, 0x304b, 0x3800, 0x3801,
        0x3802, 0x3803, 0x3808, 0x3809, 0x380a, 0x380b, 0x3840, 0x3841, 0x3842, 0x3843, 0x3848,
        0x3849, 0x384a, 0x384b, 0x0000, 0x0001, 0x0002, 0x0003, 0x0008, 0x0009, 0x000a, 0x000b,
        0x0040, 0x0041, 0x0042, 0x0043, 0x0048, 0x0049, 0x004a, 0x004b, 0x0800, 0x0801, 0x0802,
        0x0803, 0x0808, 0x0809, 0x080a, 0x080b, 0x0840, 0x0841, 0x0842, 0x0843, 0x0848, 0x0849,
        0x084a, 0x084b, 0x1000, 0x1001, 0x1002, 0x1003, 0x1008, 0x1009, 0x100a, 0x100b, 0x1040,
        0x1041, 0x1042, 0x1043, 0x1048, 0x1049, 0x104a, 0x104b, 0x1800, 0x1801, 0x1802, 0x1803,
        0x1808, 0x1809, 0x180a, 0x180b, 0x1840, 0x1841, 0x1842, 0x1843, 0x1848, 0x1849, 0x184a,
        0x184b, 0x2000, 0x2001, 0x2002, 0x2003, 0x2008, 0x2009, 0x200a, 0x200b, 0x2040, 0x2041,
        0x2042, 0x2043, 0x2048, 0x2049, 0x204a, 0x204b, 0x2800, 0x2801, 0x2802, 0x2803, 0x2808,
        0x2809, 0x280a, 0x280b, 0x2840, 0x2841, 0x2842, 0x2843, 0x2848, 0x2849, 0x284a, 0x284b,
        0x3000, 0x3001, 0x3002, 0x3003, 0x3008, 0x3009, 0x300a, 0x300b, 0x3040, 0x3041, 0x3042,
        0x3043, 0x3048, 0x3049, 0x304a, 0x304b, 0x3800, 0x3801, 0x3802, 0x3803, 0x3808, 0x3809,
        0x380a, 0x380b, 0x3840, 0x3841, 0x3842, 0x3843, 0x3848, 0x3849, 0x384a, 0x384b,
    };
    static constexpr u32 ylut[] = {
        0x0000, 0x0004, 0x0010, 0x0014, 0x0020, 0x0024, 0x0030, 0x0034, 0x0080, 0x0084, 0x0090,
        0x0094, 0x00a0, 0x00a4, 0x00b0, 0x00b4, 0x0100, 0x0104, 0x0110, 0x0114, 0x0120, 0x0124,
        0x0130, 0x0134, 0x0180, 0x0184, 0x0190, 0x0194, 0x01a0, 0x01a4, 0x01b0, 0x01b4, 0x0200,
        0x0204, 0x0210, 0x0214, 0x0220, 0x0224, 0x0230, 0x0234, 0x0280, 0x0284, 0x0290, 0x0294,
        0x02a0, 0x02a4, 0x02b0, 0x02b4, 0x0300, 0x0304, 0x0310, 0x0314, 0x0320, 0x0324, 0x0330,
        0x0334, 0x0380, 0x0384, 0x0390, 0x0394, 0x03a0, 0x03a4, 0x03b0, 0x03b4, 0x0400, 0x0404,
        0x0410, 0x0414, 0x0420, 0x0424, 0x0430, 0x0434, 0x0480, 0x0484, 0x0490, 0x0494, 0x04a0,
        0x04a4, 0x04b0, 0x04b4, 0x0500, 0x0504, 0x0510, 0x0514, 0x0520, 0x0524, 0x0530, 0x0534,
        0x0580, 0x0584, 0x0590, 0x0594, 0x05a0, 0x05a4, 0x05b0, 0x05b4, 0x0600, 0x0604, 0x0610,
        0x0614, 0x0620, 0x0624, 0x0630, 0x0634, 0x0680, 0x0684, 0x0690, 0x0694, 0x06a0, 0x06a4,
        0x06b0, 0x06b4, 0x0700, 0x0704, 0x0710, 0x0714, 0x0720, 0x0724, 0x0730, 0x0734, 0x0780,
        0x0784, 0x0790, 0x0794, 0x07a0, 0x07a4, 0x07b0, 0x07b4, 0x0000, 0x0004, 0x0010, 0x0014,
        0x0020, 0x0024, 0x0030, 0x0034, 0x0080, 0x0084, 0x0090, 0x0094, 0x00a0, 0x00a4, 0x00b0,
        0x00b4, 0x0100, 0x0104, 0x0110, 0x0114, 0x0120, 0x0124, 0x0130, 0x0134, 0x0180, 0x0184,
        0x0190, 0x0194, 0x01a0, 0x01a4, 0x01b0, 0x01b4, 0x0200, 0x0204, 0x0210, 0x0214, 0x0220,
        0x0224, 0x0230, 0x0234, 0x0280, 0x0284, 0x0290, 0x0294, 0x02a0, 0x02a4, 0x02b0, 0x02b4,
        0x0300, 0x0304, 0x0310, 0x0314, 0x0320, 0x0324, 0x0330, 0x0334, 0x0380, 0x0384, 0x0390,
        0x0394, 0x03a0, 0x03a4, 0x03b0, 0x03b4, 0x0400, 0x0404, 0x0410, 0x0414, 0x0420, 0x0424,
        0x0430, 0x0434, 0x0480, 0x0484, 0x0490, 0x0494, 0x04a0, 0x04a4, 0x04b0, 0x04b4, 0x0500,
        0x0504, 0x0510, 0x0514, 0x0520, 0x0524, 0x0530, 0x0534, 0x0580, 0x0584, 0x0590, 0x0594,
        0x05a0, 0x05a4, 0x05b0, 0x05b4, 0x0600, 0x0604, 0x0610, 0x0614, 0x0620, 0x0624, 0x0630,
        0x0634, 0x0680, 0x0684, 0x0690, 0x0694, 0x06a0, 0x06a4, 0x06b0, 0x06b4, 0x0700, 0x0704,
        0x0710, 0x0714, 0x0720, 0x0724, 0x0730, 0x0734, 0x0780, 0x0784, 0x0790, 0x0794, 0x07a0,
        0x07a4, 0x07b0, 0x07b4, 0x0000, 0x0004, 0x0010, 0x0014, 0x0020, 0x0024, 0x0030, 0x0034,
        0x0080, 0x0084, 0x0090, 0x0094, 0x00a0, 0x00a4, 0x00b0, 0x00b4, 0x0100, 0x0104, 0x0110,
        0x0114, 0x0120, 0x0124, 0x0130, 0x0134, 0x0180, 0x0184, 0x0190, 0x0194, 0x01a0, 0x01a4,
        0x01b0, 0x01b4, 0x0200, 0x0204, 0x0210, 0x0214, 0x0220, 0x0224, 0x0230, 0x0234, 0x0280,
        0x0284, 0x0290, 0x0294, 0x02a0, 0x02a4, 0x02b0, 0x02b4, 0x0300, 0x0304, 0x0310, 0x0314,
        0x0320, 0x0324, 0x0330, 0x0334, 0x0380, 0x0384, 0x0390, 0x0394, 0x03a0, 0x03a4, 0x03b0,
        0x03b4, 0x0400, 0x0404, 0x0410, 0x0414, 0x0420, 0x0424, 0x0430, 0x0434, 0x0480, 0x0484,
        0x0490, 0x0494, 0x04a0, 0x04a4, 0x04b0, 0x04b4, 0x0500, 0x0504, 0x0510, 0x0514, 0x0520,
        0x0524, 0x0530, 0x0534, 0x0580, 0x0584, 0x0590, 0x0594, 0x05a0, 0x05a4, 0x05b0, 0x05b4,
        0x0600, 0x0604, 0x0610, 0x0614, 0x0620, 0x0624, 0x0630, 0x0634, 0x0680, 0x0684, 0x0690,
        0x0694, 0x06a0, 0x06a4, 0x06b0, 0x06b4, 0x0700, 0x0704, 0x0710, 0x0714, 0x0720, 0x0724,
        0x0730, 0x0734, 0x0780, 0x0784, 0x0790, 0x0794, 0x07a0, 0x07a4, 0x07b0, 0x07b4,
    };
    return xlut[x % 128] + ylut[y % 128];
}

static inline u32 GetMortonOffset128(u32 x, u32 y, u32 bytes_per_pixel) {
    // Calculates the offset of the position of the pixel in Morton order
    // Framebuffer images are split into 128x128 tiles.

    const unsigned int block_height = 128;
    const unsigned int coarse_x = x & ~127;

    u32 i = MortonInterleave128(x, y);

    const unsigned int offset = coarse_x * block_height;

    return (i + offset) * bytes_per_pixel;
}

static void MortonCopyPixels128(u32 width, u32 height, u32 bytes_per_pixel, u32 gl_bytes_per_pixel,
                                u8* morton_data, u8* gl_data, bool morton_to_gl) {
    u8* data_ptrs[2];
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            const u32 coarse_y = y & ~127;
            u32 morton_offset =
                GetMortonOffset128(x, y, bytes_per_pixel) + coarse_y * width * bytes_per_pixel;
            u32 gl_pixel_index = (x + (height - 1 - y) * width) * gl_bytes_per_pixel;

            data_ptrs[morton_to_gl] = morton_data + morton_offset;
            data_ptrs[!morton_to_gl] = &gl_data[gl_pixel_index];

            memcpy(data_ptrs[0], data_ptrs[1], bytes_per_pixel);
        }
    }
}

/**
 * Loads framebuffer from emulated memory into the active OpenGL texture.
 */
void RendererOpenGL::LoadFBToScreenInfo(const FramebufferInfo& framebuffer_info,
                                        ScreenInfo& screen_info) {
    const u32 bpp{FramebufferInfo::BytesPerPixel(framebuffer_info.pixel_format)};
    const u32 size_in_bytes{framebuffer_info.stride * framebuffer_info.height * bpp};

    MortonCopyPixels128(framebuffer_info.width, framebuffer_info.height, bpp, 4,
                        Memory::GetPointer(framebuffer_info.address), gl_framebuffer_data.data(),
                        true);

    LOG_TRACE(Render_OpenGL, "0x%08x bytes from 0x%llx(%dx%d), fmt %x", size_in_bytes,
              framebuffer_info.address, framebuffer_info.width, framebuffer_info.height,
              (int)framebuffer_info.pixel_format);

    // Ensure no bad interactions with GL_UNPACK_ALIGNMENT, which by default
    // only allows rows to have a memory alignement of 4.
    ASSERT(framebuffer_info.stride % 4 == 0);

    framebuffer_flip_vertical = framebuffer_info.flip_vertical;

    // Reset the screen info's display texture to its own permanent texture
    screen_info.display_texture = screen_info.texture.resource.handle;
    screen_info.display_texcoords = MathUtil::Rectangle<float>(0.f, 0.f, 1.f, 1.f);

    // Memory::RasterizerFlushRegion(framebuffer_info.address, size_in_bytes);

    state.texture_units[0].texture_2d = screen_info.texture.resource.handle;
    state.Apply();

    glActiveTexture(GL_TEXTURE0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)framebuffer_info.stride);

    // Update existing texture
    // TODO: Test what happens on hardware when you change the framebuffer dimensions so that
    //       they differ from the LCD resolution.
    // TODO: Applications could theoretically crash Citra here by specifying too large
    //       framebuffer sizes. We should make sure that this cannot happen.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framebuffer_info.width, framebuffer_info.height,
                    screen_info.texture.gl_format, screen_info.texture.gl_type,
                    gl_framebuffer_data.data());

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    state.texture_units[0].texture_2d = 0;
    state.Apply();
}

/**
 * Fills active OpenGL texture with the given RGB color. Since the color is solid, the texture can
 * be 1x1 but will stretch across whatever it's rendered on.
 */
void RendererOpenGL::LoadColorToActiveGLTexture(u8 color_r, u8 color_g, u8 color_b, u8 color_a,
                                                const TextureInfo& texture) {
    state.texture_units[0].texture_2d = texture.resource.handle;
    state.Apply();

    glActiveTexture(GL_TEXTURE0);
    u8 framebuffer_data[4] = {color_a, color_b, color_g, color_r};

    // Update existing texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer_data);

    state.texture_units[0].texture_2d = 0;
    state.Apply();
}

/**
 * Initializes the OpenGL state and creates persistent objects.
 */
void RendererOpenGL::InitOpenGLObjects() {
    glClearColor(Settings::values.bg_red, Settings::values.bg_green, Settings::values.bg_blue,
                 0.0f);

    // Link shaders and get variable locations
    shader.Create(vertex_shader, fragment_shader);
    state.draw.shader_program = shader.handle;
    state.Apply();
    uniform_modelview_matrix = glGetUniformLocation(shader.handle, "modelview_matrix");
    uniform_color_texture = glGetUniformLocation(shader.handle, "color_texture");
    attrib_position = glGetAttribLocation(shader.handle, "vert_position");
    attrib_tex_coord = glGetAttribLocation(shader.handle, "vert_tex_coord");

    // Generate VBO handle for drawing
    vertex_buffer.Create();

    // Generate VAO
    vertex_array.Create();

    state.draw.vertex_array = vertex_array.handle;
    state.draw.vertex_buffer = vertex_buffer.handle;
    state.draw.uniform_buffer = 0;
    state.Apply();

    // Attach vertex data to VAO
    glBufferData(GL_ARRAY_BUFFER, sizeof(ScreenRectVertex) * 4, nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenRectVertex),
                          (GLvoid*)offsetof(ScreenRectVertex, position));
    glVertexAttribPointer(attrib_tex_coord, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenRectVertex),
                          (GLvoid*)offsetof(ScreenRectVertex, tex_coord));
    glEnableVertexAttribArray(attrib_position);
    glEnableVertexAttribArray(attrib_tex_coord);

    // Allocate textures for the screen
    screen_info.texture.resource.Create();

    // Allocation of storage is deferred until the first frame, when we
    // know the framebuffer size.

    state.texture_units[0].texture_2d = screen_info.texture.resource.handle;
    state.Apply();

    glActiveTexture(GL_TEXTURE0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    screen_info.display_texture = screen_info.texture.resource.handle;

    state.texture_units[0].texture_2d = 0;
    state.Apply();

    // Clear screen to black
    LoadColorToActiveGLTexture(0, 0, 0, 0, screen_info.texture);
}

void RendererOpenGL::ConfigureFramebufferTexture(TextureInfo& texture,
                                                 const FramebufferInfo& framebuffer_info) {

    texture.width = framebuffer_info.width;
    texture.height = framebuffer_info.height;

    GLint internal_format;
    switch (framebuffer_info.pixel_format) {
    case FramebufferInfo::PixelFormat::ABGR8:
        // Use RGBA8 and swap in the fragment shader
        internal_format = GL_RGBA;
        texture.gl_format = GL_RGBA;
        texture.gl_type = GL_UNSIGNED_INT_8_8_8_8;
        gl_framebuffer_data.resize(texture.width * texture.height * 4);
        break;
    default:
        UNIMPLEMENTED();
    }

    state.texture_units[0].texture_2d = texture.resource.handle;
    state.Apply();

    glActiveTexture(GL_TEXTURE0);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture.width, texture.height, 0,
                 texture.gl_format, texture.gl_type, nullptr);

    state.texture_units[0].texture_2d = 0;
    state.Apply();
}

void RendererOpenGL::DrawSingleScreen(const ScreenInfo& screen_info, float x, float y, float w,
                                      float h) {
    const auto& texcoords = screen_info.display_texcoords;
    const auto& left = framebuffer_flip_vertical ? texcoords.right : texcoords.left;
    const auto& right = framebuffer_flip_vertical ? texcoords.left : texcoords.right;

    std::array<ScreenRectVertex, 4> vertices = {{
        ScreenRectVertex(x, y, texcoords.top, right),
        ScreenRectVertex(x + w, y, texcoords.bottom, right),
        ScreenRectVertex(x, y + h, texcoords.top, left),
        ScreenRectVertex(x + w, y + h, texcoords.bottom, left),
    }};

    state.texture_units[0].texture_2d = screen_info.display_texture;
    state.Apply();

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices.data());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    state.texture_units[0].texture_2d = 0;
    state.Apply();
}

/**
 * Draws the emulated screens to the emulator window.
 */
void RendererOpenGL::DrawScreens() {
    const auto& layout = render_window->GetFramebufferLayout();
    const auto& screen = layout.screen;

    glViewport(0, 0, layout.width, layout.height);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set projection matrix
    std::array<GLfloat, 3 * 2> ortho_matrix =
        MakeOrthographicMatrix((float)layout.width, (float)layout.height);
    glUniformMatrix3x2fv(uniform_modelview_matrix, 1, GL_FALSE, ortho_matrix.data());

    // Bind texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(uniform_color_texture, 0);

    DrawSingleScreen(screen_info, (float)screen.left, (float)screen.top, (float)screen.GetWidth(),
                     (float)screen.GetHeight());

    m_current_frame++;
}

/// Updates the framerate
void RendererOpenGL::UpdateFramerate() {}

/**
 * Set the emulator window to use for renderer
 * @param window EmuWindow handle to emulator window to use for rendering
 */
void RendererOpenGL::SetWindow(EmuWindow* window) {
    render_window = window;
}

static const char* GetSource(GLenum source) {
#define RET(s)                                                                                     \
    case GL_DEBUG_SOURCE_##s:                                                                      \
        return #s
    switch (source) {
        RET(API);
        RET(WINDOW_SYSTEM);
        RET(SHADER_COMPILER);
        RET(THIRD_PARTY);
        RET(APPLICATION);
        RET(OTHER);
    default:
        UNREACHABLE();
    }
#undef RET
}

static const char* GetType(GLenum type) {
#define RET(t)                                                                                     \
    case GL_DEBUG_TYPE_##t:                                                                        \
        return #t
    switch (type) {
        RET(ERROR);
        RET(DEPRECATED_BEHAVIOR);
        RET(UNDEFINED_BEHAVIOR);
        RET(PORTABILITY);
        RET(PERFORMANCE);
        RET(OTHER);
        RET(MARKER);
    default:
        UNREACHABLE();
    }
#undef RET
}

static void APIENTRY DebugHandler(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar* message, const void* user_param) {
    Log::Level level;
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        level = Log::Level::Error;
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        level = Log::Level::Warning;
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
    case GL_DEBUG_SEVERITY_LOW:
        level = Log::Level::Debug;
        break;
    }
    LOG_GENERIC(Log::Class::Render_OpenGL, level, "%s %s %d: %s", GetSource(source), GetType(type),
                id, message);
}

/// Initialize the renderer
bool RendererOpenGL::Init() {
    render_window->MakeCurrent();

    if (GLAD_GL_KHR_debug) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(DebugHandler, nullptr);
    }

    const char* gl_version{reinterpret_cast<char const*>(glGetString(GL_VERSION))};
    const char* gpu_vendor{reinterpret_cast<char const*>(glGetString(GL_VENDOR))};
    const char* gpu_model{reinterpret_cast<char const*>(glGetString(GL_RENDERER))};

    LOG_INFO(Render_OpenGL, "GL_VERSION: %s", gl_version);
    LOG_INFO(Render_OpenGL, "GL_VENDOR: %s", gpu_vendor);
    LOG_INFO(Render_OpenGL, "GL_RENDERER: %s", gpu_model);

    Core::Telemetry().AddField(Telemetry::FieldType::UserSystem, "GPU_Vendor", gpu_vendor);
    Core::Telemetry().AddField(Telemetry::FieldType::UserSystem, "GPU_Model", gpu_model);
    Core::Telemetry().AddField(Telemetry::FieldType::UserSystem, "GPU_OpenGL_Version", gl_version);

    if (!GLAD_GL_VERSION_3_3) {
        return false;
    }

    InitOpenGLObjects();

    RefreshRasterizerSetting();

    return true;
}

/// Shutdown the renderer
void RendererOpenGL::ShutDown() {}
