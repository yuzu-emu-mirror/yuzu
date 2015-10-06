// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/pica.h"
#include "video_core/renderer_opengl/gl_rasterizer.h"
#include "video_core/renderer_opengl/gl_shader_gen.h"

using Pica::Regs;
using TevStageConfig = Regs::TevStageConfig;

namespace GLShader {

/// Detects if a TEV stage is configured to be skipped (to avoid generating unnecessary code)
static bool IsPassThroughTevStage(const TevStageConfig& stage) {
    return (stage.color_op             == TevStageConfig::Operation::Replace &&
            stage.alpha_op             == TevStageConfig::Operation::Replace &&
            stage.color_source1        == TevStageConfig::Source::Previous &&
            stage.alpha_source1        == TevStageConfig::Source::Previous &&
            stage.color_modifier1      == TevStageConfig::ColorModifier::SourceColor &&
            stage.alpha_modifier1      == TevStageConfig::AlphaModifier::SourceAlpha &&
            stage.GetColorMultiplier() == 1 &&
            stage.GetAlphaMultiplier() == 1);
}

/// Writes the specified TEV stage source component(s)
static void AppendSource(std::string& out, TevStageConfig::Source source,
        const std::string& index_name) {
    using Source = TevStageConfig::Source;
    switch (source) {
    case Source::PrimaryColor:
        out += "o[2]";
        break;
    case Source::PrimaryFragmentColor:
        // HACK: Until we implement fragment lighting, use primary_color
        out += "o[2]";
        break;
    case Source::SecondaryFragmentColor:
        // HACK: Until we implement fragment lighting, use zero
        out += "vec4(0.0, 0.0, 0.0, 0.0)";
        break;
    case Source::Texture0:
        out += "texture(tex[0], o[3].xy)";
        break;
    case Source::Texture1:
        out += "texture(tex[1], o[3].zw)";
        break;
    case Source::Texture2: // TODO: Unverified
        out += "texture(tex[2], o[5].zw)";
        break;
    case Source::PreviousBuffer:
        out += "g_combiner_buffer";
        break;
    case Source::Constant:
        out += "const_color[" + index_name + "]";
        break;
    case Source::Previous:
        out += "g_last_tex_env_out";
        break;
    default:
        out += "vec4(0.0)";
        LOG_CRITICAL(Render_OpenGL, "Unknown source op %u", source);
        break;
    }
}

/// Writes the color components to use for the specified TEV stage color modifier
static void AppendColorModifier(std::string& out, TevStageConfig::ColorModifier modifier,
        TevStageConfig::Source source, const std::string& index_name) {
    using ColorModifier = TevStageConfig::ColorModifier;
    switch (modifier) {
    case ColorModifier::SourceColor:
        AppendSource(out, source, index_name);
        out += ".rgb";
        break;
    case ColorModifier::OneMinusSourceColor:
        out += "vec3(1.0) - ";
        AppendSource(out, source, index_name);
        out += ".rgb";
        break;
    case ColorModifier::SourceAlpha:
        AppendSource(out, source, index_name);
        out += ".aaa";
        break;
    case ColorModifier::OneMinusSourceAlpha:
        out += "vec3(1.0) - ";
        AppendSource(out, source, index_name);
        out += ".aaa";
        break;
    case ColorModifier::SourceRed:
        AppendSource(out, source, index_name);
        out += ".rrr";
        break;
    case ColorModifier::OneMinusSourceRed:
        out += "vec3(1.0) - ";
        AppendSource(out, source, index_name);
        out += ".rrr";
        break;
    case ColorModifier::SourceGreen:
        AppendSource(out, source, index_name);
        out += ".ggg";
        break;
    case ColorModifier::OneMinusSourceGreen:
        out += "vec3(1.0) - ";
        AppendSource(out, source, index_name);
        out += ".ggg";
        break;
    case ColorModifier::SourceBlue:
        AppendSource(out, source, index_name);
        out += ".bbb";
        break;
    case ColorModifier::OneMinusSourceBlue:
        out += "vec3(1.0) - ";
        AppendSource(out, source, index_name);
        out += ".bbb";
        break;
    default:
        out += "vec3(0.0)";
        LOG_CRITICAL(Render_OpenGL, "Unknown color modifier op %u", modifier);
        break;
    }
}

/// Writes the alpha component to use for the specified TEV stage alpha modifier
static void AppendAlphaModifier(std::string& out, TevStageConfig::AlphaModifier modifier,
        TevStageConfig::Source source, const std::string& index_name) {
    using AlphaModifier = TevStageConfig::AlphaModifier;
    switch (modifier) {
    case AlphaModifier::SourceAlpha:
        AppendSource(out, source, index_name);
        out += ".a";
        break;
    case AlphaModifier::OneMinusSourceAlpha:
        out += "1.0 - ";
        AppendSource(out, source, index_name);
        out += ".a";
        break;
    case AlphaModifier::SourceRed:
        AppendSource(out, source, index_name);
        out += ".r";
        break;
    case AlphaModifier::OneMinusSourceRed:
        out += "1.0 - ";
        AppendSource(out, source, index_name);
        out += ".r";
        break;
    case AlphaModifier::SourceGreen:
        AppendSource(out, source, index_name);
        out += ".g";
        break;
    case AlphaModifier::OneMinusSourceGreen:
        out += "1.0 - ";
        AppendSource(out, source, index_name);
        out += ".g";
        break;
    case AlphaModifier::SourceBlue:
        AppendSource(out, source, index_name);
        out += ".b";
        break;
    case AlphaModifier::OneMinusSourceBlue:
        out += "1.0 - ";
        AppendSource(out, source, index_name);
        out += ".b";
        break;
    default:
        out += "vec3(0.0)";
        LOG_CRITICAL(Render_OpenGL, "Unknown alpha modifier op %u", modifier);
        break;
    }
}

/// Writes the combiner function for the color components for the specified TEV stage operation
static void AppendColorCombiner(std::string& out, TevStageConfig::Operation operation,
        const std::string& variable_name) {
    using Operation = TevStageConfig::Operation;

    switch (operation) {
    case Operation::Replace:
        out += variable_name + "[0]";
        break;
    case Operation::Modulate:
        out += variable_name + "[0] * " + variable_name + "[1]";
        break;
    case Operation::Add:
        out += "min(" + variable_name + "[0] + " + variable_name + "[1], vec3(1.0))";
        break;
    case Operation::AddSigned:
        out += "clamp(" + variable_name + "[0] + " + variable_name + "[1] - vec3(0.5), vec3(0.0), vec3(1.0))";
        break;
    case Operation::Lerp:
        out += variable_name + "[0] * " + variable_name + "[2] + " + variable_name + "[1] * (vec3(1.0) - " + variable_name + "[2])";
        break;
    case Operation::Subtract:
        out += "max(" + variable_name + "[0] - " + variable_name + "[1], vec3(0.0))";
        break;
    case Operation::MultiplyThenAdd:
        out += "min(" + variable_name + "[0] * " + variable_name + "[1] + " + variable_name + "[2], vec3(1.0))";
        break;
    case Operation::AddThenMultiply:
        out += "min(" + variable_name + "[0] + " + variable_name + "[1], vec3(1.0)) * " + variable_name + "[2]";
        break;
    default:
        out += "vec3(0.0)";
        LOG_CRITICAL(Render_OpenGL, "Unknown color combiner operation: %u", operation);
        break;
    }
}

/// Writes the combiner function for the alpha component for the specified TEV stage operation
static void AppendAlphaCombiner(std::string& out, TevStageConfig::Operation operation,
        const std::string& variable_name) {
    using Operation = TevStageConfig::Operation;
    switch (operation) {
    case Operation::Replace:
        out += variable_name + "[0]";
        break;
    case Operation::Modulate:
        out += variable_name + "[0] * " + variable_name + "[1]";
        break;
    case Operation::Add:
        out += "min(" + variable_name + "[0] + " + variable_name + "[1], 1.0)";
        break;
    case Operation::AddSigned:
        out += "clamp(" + variable_name + "[0] + " + variable_name + "[1] - 0.5, 0.0, 1.0)";
        break;
    case Operation::Lerp:
        out += variable_name + "[0] * " + variable_name + "[2] + " + variable_name + "[1] * (1.0 - " + variable_name + "[2])";
        break;
    case Operation::Subtract:
        out += "max(" + variable_name + "[0] - " + variable_name + "[1], 0.0)";
        break;
    case Operation::MultiplyThenAdd:
        out += "min(" + variable_name + "[0] * " + variable_name + "[1] + " + variable_name + "[2], 1.0)";
        break;
    case Operation::AddThenMultiply:
        out += "min(" + variable_name + "[0] + " + variable_name + "[1], 1.0) * " + variable_name + "[2]";
        break;
    default:
        out += "0.0";
        LOG_CRITICAL(Render_OpenGL, "Unknown alpha combiner operation: %u", operation);
        break;
    }
}

/// Writes the if-statement condition used to evaluate alpha testing
static void AppendAlphaTestCondition(std::string& out, Regs::CompareFunc func) {
    using CompareFunc = Regs::CompareFunc;
    switch (func) {
    case CompareFunc::Never:
        out += "true";
        break;
    case CompareFunc::Always:
        out += "false";
        break;
    case CompareFunc::Equal:
        out += "int(g_last_tex_env_out.a * 255.0f) != alphatest_ref";
        break;
    case CompareFunc::NotEqual:
        out += "int(g_last_tex_env_out.a * 255.0f) == alphatest_ref";
        break;
    case CompareFunc::LessThan:
        out += "int(g_last_tex_env_out.a * 255.0f) >= alphatest_ref";
        break;
    case CompareFunc::LessThanOrEqual:
        out += "int(g_last_tex_env_out.a * 255.0f) > alphatest_ref";
        break;
    case CompareFunc::GreaterThan:
        out += "int(g_last_tex_env_out.a * 255.0f) <= alphatest_ref";
        break;
    case CompareFunc::GreaterThanOrEqual:
        out += "int(g_last_tex_env_out.a * 255.0f) < alphatest_ref";
        break;
    default:
        out += "false";
        LOG_CRITICAL(Render_OpenGL, "Unknown alpha test condition %u", func);
        break;
    }
}

/// Writes the code to emulate the specified TEV stage
static void WriteTevStage(std::string& out, const ShaderCacheKey& config, unsigned index) {
    auto& stage = config.tev_stages[index];
    if (!IsPassThroughTevStage(stage)) {
        std::string index_name = std::to_string(index);

        out += "vec3 color_results_" + index_name + "[3] = vec3[3](";
        AppendColorModifier(out, stage.color_modifier1, stage.color_source1, index_name);
        out += ", ";
        AppendColorModifier(out, stage.color_modifier2, stage.color_source2, index_name);
        out += ", ";
        AppendColorModifier(out, stage.color_modifier3, stage.color_source3, index_name);
        out += ");\n";

        out += "vec3 color_output_" + index_name + " = ";
        AppendColorCombiner(out, stage.color_op, "color_results_" + index_name);
        out += ";\n";

        out += "float alpha_results_" + index_name + "[3] = float[3](";
        AppendAlphaModifier(out, stage.alpha_modifier1, stage.alpha_source1, index_name);
        out += ", ";
        AppendAlphaModifier(out, stage.alpha_modifier2, stage.alpha_source2, index_name);
        out += ", ";
        AppendAlphaModifier(out, stage.alpha_modifier3, stage.alpha_source3, index_name);
        out += ");\n";

        out += "float alpha_output_" + index_name + " = ";
        AppendAlphaCombiner(out, stage.alpha_op, "alpha_results_" + index_name);
        out += ";\n";

        out += "g_last_tex_env_out = vec4(min(color_output_" + index_name + " * " +
            std::to_string(stage.GetColorMultiplier()) + ".0, 1.0), min(alpha_output_" + index_name + " * " +
            std::to_string(stage.GetAlphaMultiplier()) + ".0, 1.0));\n";
    }

    if (config.TevStageUpdatesCombinerBufferColor(index))
        out += "g_combiner_buffer.rgb = g_last_tex_env_out.rgb;\n";

    if (config.TevStageUpdatesCombinerBufferAlpha(index))
        out += "g_combiner_buffer.a = g_last_tex_env_out.a;\n";
}

std::string GenerateFragmentShader(const ShaderCacheKey& config) {
    std::string out = R"(
#version 150 core

#define NUM_VTX_ATTR 7
#define NUM_TEV_STAGES 6

in vec4 o[NUM_VTX_ATTR];
out vec4 color;

uniform int alphatest_ref;
uniform vec4 const_color[NUM_TEV_STAGES];
uniform sampler2D tex[3];

uniform vec4 tev_combiner_buffer_color;

void main(void) {
vec4 g_combiner_buffer = tev_combiner_buffer_color;
vec4 g_last_tex_env_out = vec4(0.0, 0.0, 0.0, 0.0);
)";

    // Do not do any sort of processing if it's obvious we're not going to pass the alpha test
    if (config.alpha_test_func == Regs::CompareFunc::Never) {
        out += "discard;";
        return out;
    }

    for (std::size_t index = 0; index < config.tev_stages.size(); ++index)
        WriteTevStage(out, config, (unsigned)index);

    if (config.alpha_test_func != Regs::CompareFunc::Always) {
        out += "if (";
        AppendAlphaTestCondition(out, config.alpha_test_func);
        out += ") {\n discard;\n }\n";
    }

    out += "color = g_last_tex_env_out;\n}";

    return out;
}

std::string GenerateVertexShader() {
    static const std::string out = R"(
#version 150 core

#define NUM_VTX_ATTR 7

in vec4 vert_position;
in vec4 vert_color;
in vec2 vert_texcoords0;
in vec2 vert_texcoords1;
in vec2 vert_texcoords2;

out vec4 o[NUM_VTX_ATTR];

void main() {
    o[2] = vert_color;
    o[3] = vec4(vert_texcoords0.xy, vert_texcoords1.xy);
    o[5] = vec4(0.0, 0.0, vert_texcoords2.xy);

    gl_Position = vec4(vert_position.x, -vert_position.y, -vert_position.z, vert_position.w);
}
)";
    return out;
}

} // namespace GLShader
