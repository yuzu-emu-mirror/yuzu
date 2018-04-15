// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include <glad/glad.h>
#include "common/assert.h"
#include "common/logging/log.h"

namespace GLShader {

/**
 * Utility function to create and compile an OpenGL GLSL shader
 * @param source String of the GLSL shader program
 * @param type Type of the shader (GL_VERTEX_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER)
 */
GLuint LoadShader(const char* source, GLenum type);

/**
 * Utility function to create and compile an OpenGL GLSL shader program (vertex + fragment shader)
 * @param separable_program whether to create a separable program
 * @param shaders ID of shaders to attach to the program
 * @returns Handle of the newly created OpenGL program object
 */
template <typename... T>
GLuint LoadProgram(bool separable_program, T... shaders) {
    // Link the program
    NGLOG_DEBUG(Render_OpenGL, "Linking program...");

    GLuint program_id = glCreateProgram();

    ((shaders == 0 ? (void)0 : glAttachShader(program_id, shaders)), ...);

    if (separable_program) {
        glProgramParameteri(program_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
    }

    glLinkProgram(program_id);

    // Check the program
    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::string program_error(info_log_length, ' ');
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &program_error[0]);
        if (result == GL_TRUE) {
            NGLOG_DEBUG(Render_OpenGL, "{}", program_error);
        } else {
            NGLOG_ERROR(Render_OpenGL, "Error linking shader:\n{}", program_error);
        }
    }

    ASSERT_MSG(result == GL_TRUE, "Shader not linked");

    ((shaders == 0 ? (void)0 : glDetachShader(program_id, shaders)), ...);

    return program_id;
}

} // namespace GLShader
