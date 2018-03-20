// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>
#include <glad/glad.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_core/renderer_opengl/gl_shader_util.h"

namespace GLShader {

GLuint LoadProgram(const char* vertex_shader, const char* geometry_shader,
                   const char* fragment_shader, const std::vector<const char*>& feedback_vars,
                   bool separable_program) {
    // Create the shaders
    GLuint vertex_shader_id = vertex_shader ? glCreateShader(GL_VERTEX_SHADER) : 0;
    GLuint geometry_shader_id = geometry_shader ? glCreateShader(GL_GEOMETRY_SHADER) : 0;
    GLuint fragment_shader_id = fragment_shader ? glCreateShader(GL_FRAGMENT_SHADER) : 0;

    GLint result = GL_FALSE;
    int info_log_length;

    if (vertex_shader) {
        // Compile Vertex Shader
        LOG_DEBUG(Render_OpenGL, "Compiling vertex shader...");

        glShaderSource(vertex_shader_id, 1, &vertex_shader, nullptr);
        glCompileShader(vertex_shader_id);

        // Check Vertex Shader
        glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
        glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

        if (info_log_length > 1) {
            std::vector<char> vertex_shader_error(info_log_length);
            glGetShaderInfoLog(vertex_shader_id, info_log_length, nullptr, &vertex_shader_error[0]);
            if (result == GL_TRUE) {
                LOG_DEBUG(Render_OpenGL, "%s", &vertex_shader_error[0]);
            } else {
                LOG_ERROR(Render_OpenGL, "Error compiling vertex shader:\n%s",
                          &vertex_shader_error[0]);
            }
        }
    }

    if (geometry_shader) {
        // Compile Geometry Shader
        LOG_DEBUG(Render_OpenGL, "Compiling geometry shader...");

        glShaderSource(geometry_shader_id, 1, &geometry_shader, nullptr);
        glCompileShader(geometry_shader_id);

        // Check Geometry Shader
        glGetShaderiv(geometry_shader_id, GL_COMPILE_STATUS, &result);
        glGetShaderiv(geometry_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

        if (info_log_length > 1) {
            std::vector<char> geometry_shader_error(info_log_length);
            glGetShaderInfoLog(geometry_shader_id, info_log_length, nullptr,
                               &geometry_shader_error[0]);
            if (result == GL_TRUE) {
                LOG_DEBUG(Render_OpenGL, "%s", &geometry_shader_error[0]);
            } else {
                LOG_ERROR(Render_OpenGL, "Error compiling geometry shader:\n%s",
                          &geometry_shader_error[0]);
            }
        }
    }

    if (fragment_shader) {
        // Compile Fragment Shader
        LOG_DEBUG(Render_OpenGL, "Compiling fragment shader...");

        glShaderSource(fragment_shader_id, 1, &fragment_shader, nullptr);
        glCompileShader(fragment_shader_id);

        // Check Fragment Shader
        glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
        glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

        if (info_log_length > 1) {
            std::vector<char> fragment_shader_error(info_log_length);
            glGetShaderInfoLog(fragment_shader_id, info_log_length, nullptr,
                               &fragment_shader_error[0]);
            if (result == GL_TRUE) {
                LOG_DEBUG(Render_OpenGL, "%s", &fragment_shader_error[0]);
            } else {
                LOG_ERROR(Render_OpenGL, "Error compiling fragment shader:\n%s",
                          &fragment_shader_error[0]);
            }
        }
    }

    // Link the program
    LOG_DEBUG(Render_OpenGL, "Linking program...");

    GLuint program_id = glCreateProgram();
    if (vertex_shader) {
        glAttachShader(program_id, vertex_shader_id);
    }
    if (geometry_shader) {
        glAttachShader(program_id, geometry_shader_id);
    }
    if (fragment_shader) {
        glAttachShader(program_id, fragment_shader_id);
    }

    if (!feedback_vars.empty()) {
        auto varyings = feedback_vars;
        glTransformFeedbackVaryings(program_id, static_cast<GLsizei>(feedback_vars.size()),
                                    &varyings[0], GL_INTERLEAVED_ATTRIBS);
    }

    if (separable_program) {
        glProgramParameteri(program_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
    }

    glLinkProgram(program_id);

    // Check the program
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::vector<char> program_error(info_log_length);
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &program_error[0]);
        if (result == GL_TRUE) {
            LOG_DEBUG(Render_OpenGL, "%s", &program_error[0]);
        } else {
            LOG_ERROR(Render_OpenGL, "Error linking shader:\n%s", &program_error[0]);
        }
    }

    // If the program linking failed at least one of the shaders was probably bad
    if (result == GL_FALSE) {
        if (vertex_shader) {
            LOG_ERROR(Render_OpenGL, "Vertex shader:\n%s", vertex_shader);
        }
        if (geometry_shader) {
            LOG_ERROR(Render_OpenGL, "Geometry shader:\n%s", geometry_shader);
        }
        if (fragment_shader) {
            LOG_ERROR(Render_OpenGL, "Fragment shader:\n%s", fragment_shader);
        }
    }
    ASSERT_MSG(result == GL_TRUE, "Shader not linked");

    if (vertex_shader) {
        glDetachShader(program_id, vertex_shader_id);
        glDeleteShader(vertex_shader_id);
    }
    if (geometry_shader) {
        glDetachShader(program_id, geometry_shader_id);
        glDeleteShader(geometry_shader_id);
    }
    if (fragment_shader) {
        glDetachShader(program_id, fragment_shader_id);
        glDeleteShader(fragment_shader_id);
    }

    return program_id;
}

} // namespace GLShader
