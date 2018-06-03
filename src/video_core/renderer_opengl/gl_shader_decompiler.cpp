// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include <set>
#include <string>
#include <string_view>
#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/renderer_opengl/gl_shader_decompiler.h"

namespace GLShader {
namespace Decompiler {

using Tegra::Shader::Attribute;
using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::Register;
using Tegra::Shader::Sampler;
using Tegra::Shader::SubOp;
using Tegra::Shader::Uniform;

constexpr u32 PROGRAM_END = MAX_PROGRAM_CODE_LENGTH;

class DecompileFail : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// Describes the behaviour of code path of a given entry point and a return point.
enum class ExitMethod {
    Undetermined, ///< Internal value. Only occur when analyzing JMP loop.
    AlwaysReturn, ///< All code paths reach the return point.
    Conditional,  ///< Code path reaches the return point or an END instruction conditionally.
    AlwaysEnd,    ///< All code paths reach a END instruction.
};

/// A subroutine is a range of code refereced by a CALL, IF or LOOP instruction.
struct Subroutine {
    /// Generates a name suitable for GLSL source code.
    std::string GetName() const {
        return "sub_" + std::to_string(begin) + '_' + std::to_string(end);
    }

    u32 begin;              ///< Entry point of the subroutine.
    u32 end;                ///< Return point of the subroutine.
    ExitMethod exit_method; ///< Exit method of the subroutine.
    std::set<u32> labels;   ///< Addresses refereced by JMP instructions.

    bool operator<(const Subroutine& rhs) const {
        return std::tie(begin, end) < std::tie(rhs.begin, rhs.end);
    }
};

/// Analyzes shader code and produces a set of subroutines.
class ControlFlowAnalyzer {
public:
    ControlFlowAnalyzer(const ProgramCode& program_code, u32 main_offset)
        : program_code(program_code) {

        // Recursively finds all subroutines.
        const Subroutine& program_main = AddSubroutine(main_offset, PROGRAM_END);
        if (program_main.exit_method != ExitMethod::AlwaysEnd)
            throw DecompileFail("Program does not always end");
    }

    std::set<Subroutine> GetSubroutines() {
        return std::move(subroutines);
    }

private:
    const ProgramCode& program_code;
    std::set<Subroutine> subroutines;
    std::map<std::pair<u32, u32>, ExitMethod> exit_method_map;

    /// Adds and analyzes a new subroutine if it is not added yet.
    const Subroutine& AddSubroutine(u32 begin, u32 end) {
        auto iter = subroutines.find(Subroutine{begin, end});
        if (iter != subroutines.end())
            return *iter;

        Subroutine subroutine{begin, end};
        subroutine.exit_method = Scan(begin, end, subroutine.labels);
        if (subroutine.exit_method == ExitMethod::Undetermined)
            throw DecompileFail("Recursive function detected");
        return *subroutines.insert(std::move(subroutine)).first;
    }

    /// Scans a range of code for labels and determines the exit method.
    ExitMethod Scan(u32 begin, u32 end, std::set<u32>& labels) {
        auto [iter, inserted] =
            exit_method_map.emplace(std::make_pair(begin, end), ExitMethod::Undetermined);
        ExitMethod& exit_method = iter->second;
        if (!inserted)
            return exit_method;

        for (u32 offset = begin; offset != end && offset != PROGRAM_END; ++offset) {
            if (const auto opcode = OpCode::Decode({program_code[offset]})) {
                switch (opcode->GetId()) {
                case OpCode::Id::EXIT: {
                    return exit_method = ExitMethod::AlwaysEnd;
                }
                }
            }
        }
        return exit_method = ExitMethod::AlwaysReturn;
    }
};

class ShaderWriter {
public:
    void AddLine(std::string_view text) {
        DEBUG_ASSERT(scope >= 0);
        if (!text.empty()) {
            AppendIndentation();
        }
        shader_source += text;
        AddNewLine();
    }

    void AddLine(char character) {
        DEBUG_ASSERT(scope >= 0);
        AppendIndentation();
        shader_source += character;
        AddNewLine();
    }

    void AddNewLine() {
        DEBUG_ASSERT(scope >= 0);
        shader_source += '\n';
    }

    std::string GetResult() {
        return std::move(shader_source);
    }

    int scope = 0;

private:
    void AppendIndentation() {
        shader_source.append(static_cast<size_t>(scope) * 4, ' ');
    }

    std::string shader_source;
};

/**
 * Represents an emulated shader register, used to track the state of that register for emulation
 * with GLSL. At this time, a register can be used as a float or an integer. This class is used for
 * bookkeeping within the GLSL program.
 */
class GLSLRegister {
public:
    enum class Type {
        Float,
        Integer,
        UnsignedInteger,
    };

    GLSLRegister(size_t index, ShaderWriter& shader) : index{index}, shader{shader} {}

    /// Gets the GLSL type string for a register
    static std::string GetTypeString(Type type) {
        switch (type) {
        case Type::Float:
            return "float";
        case Type::Integer:
            return "int";
        case Type::UnsignedInteger:
            return "uint";
        }

        UNREACHABLE();
        return {};
    }

    /// Gets the GLSL register prefix string, used for declarations and referencing
    static std::string GetPrefixString(Type type) {
        return "reg_" + GetTypeString(type) + '_';
    }

    /// Returns a GLSL string representing the current state of the register
    const std::string GetActiveString() {
        declr_type.insert(active_type);
        return GetPrefixString(active_type) + std::to_string(index);
    }

    /// Returns true if the active type is a float
    bool IsFloat() const {
        return active_type == Type::Float;
    }

    /// Returns true if the active type is an integer
    bool IsInteger() const {
        return active_type == Type::Integer;
    }

    /// Returns the index of the register
    size_t GetIndex() const {
        return index;
    }

    /// Returns a set of the declared types of the register
    const std::set<Type>& DeclaredTypes() const {
        return declr_type;
    }

private:
    const size_t index;
    const std::string float_str;
    const std::string integer_str;
    ShaderWriter& shader;
    Type active_type{Type::Float};
    std::set<Type> declr_type;
};

/**
 * Used to manage shader registers that are emulated with GLSL. This class keeps track of the state
 * of all registers (e.g. whether they are currently being used as Floats or Integers), and
 * generates the necessary GLSL code to perform conversions as needed. This class is used for
 * bookkeeping within the GLSL program.
 */
class GLSLRegisterManager {
public:
    GLSLRegisterManager(ShaderWriter& shader, ShaderWriter& declarations,
                        const Maxwell3D::Regs::ShaderStage& stage)
        : shader{shader}, declarations{declarations}, stage{stage} {
        BuildRegisterList();
    }

    /**
     * Gets a register as an float.
     * @param reg The register to get.
     * @param elem The element to use for the operation.
     * @returns GLSL string corresponding to the register as a float.
     */
    std::string GetRegisterAsFloat(const Register& reg, unsigned elem = 0) {
        ASSERT(regs[reg].IsFloat());
        return GetRegister(reg, elem);
    }

    /**
     * Gets a register as an integer.
     * @param reg The register to get.
     * @param elem The element to use for the operation.
     * @param is_signed Whether to get the register as a signed (or unsigned) integer.
     * @returns GLSL string corresponding to the register as an integer.
     */
    std::string GetRegisterAsInteger(const Register& reg, unsigned elem = 0,
                                     bool is_signed = true) {
        const std::string func = GetGLSLConversionFunc(
            GLSLRegister::Type::Float,
            is_signed ? GLSLRegister::Type::Integer : GLSLRegister::Type::UnsignedInteger);

        return func + '(' + GetRegister(reg, elem) + ')';
    }

    /**
     * Writes code that does a register assignment to float value operation.
     * @param reg The destination register to use.
     * @param elem The element to use for the operation.
     * @param value The code representing the value to assign.
     * @param dest_num_components Number of components in the destination.
     * @param value_num_components Number of components in the value.
     * @param is_abs Optional, when True, applies absolute value to output.
     * @param dest_elem Optional, the destination element to use for the operation.
     */
    void SetRegisterToFloat(const Register& reg, u64 elem, const std::string& value,
                            u64 dest_num_components, u64 value_num_components, bool is_abs = false,
                            u64 dest_elem = 0) {
        SetRegister(reg, elem, value, dest_num_components, value_num_components, is_abs, dest_elem);
    }

    /**
     * Writes code that does a register assignment to integer value operation.
     * @param reg The destination register to use.
     * @param elem The element to use for the operation.
     * @param value The code representing the value to assign.
     * @param dest_num_components Number of components in the destination.
     * @param value_num_components Number of components in the value.
     * @param is_abs Optional, when True, applies absolute value to output.
     * @param dest_elem Optional, the destination element to use for the operation.
     */
    void SetRegisterToInteger(const Register& reg, bool is_signed, u64 elem,
                              const std::string& value, u64 dest_num_components,
                              u64 value_num_components, bool is_abs = false, u64 dest_elem = 0) {
        const std::string func = GetGLSLConversionFunc(
            is_signed ? GLSLRegister::Type::Integer : GLSLRegister::Type::UnsignedInteger,
            GLSLRegister::Type::Float);

        SetRegister(reg, elem, func + '(' + value + ')', dest_num_components, value_num_components,
                    is_abs, dest_elem);
    }

    /**
     * Writes code that does a register assignment to input attribute operation. Input attributes
     * are stored as floats, so this may require conversion.
     * @param reg The destination register to use.
     * @param elem The element to use for the operation.
     * @param attribute The input attribute to use as the source value.
     */
    void SetRegisterToInputAttibute(const Register& reg, u64 elem, Attribute::Index attribute) {
        std::string dest = GetRegisterAsFloat(reg);
        std::string src = GetInputAttribute(attribute) + GetSwizzle(elem);

        if (regs[reg].IsFloat()) {
            shader.AddLine(dest + " = " + src + ';');
        } else if (regs[reg].IsInteger()) {
            shader.AddLine(dest + " = floatBitsToInt(" + src + ");");
        } else {
            UNREACHABLE();
        }
    }

    /**
     * Writes code that does a output attribute assignment to register operation. Output attributes
     * are stored as floats, so this may require conversion.
     * @param attribute The destination output attribute.
     * @param elem The element to use for the operation.
     * @param reg The register to use as the source value.
     */
    void SetOutputAttributeToRegister(Attribute::Index attribute, u64 elem, const Register& reg) {
        std::string dest = GetOutputAttribute(attribute) + GetSwizzle(elem);
        std::string src = GetRegisterAsFloat(reg);
        ASSERT_MSG(regs[reg].IsFloat(), "Output attributes must be set to a float");
        shader.AddLine(dest + " = " + src + ';');
    }

    /// Generates code representing a uniform (C buffer) register.
    std::string GetUniform(const Uniform& uniform, const Register& dest_reg) {
        declr_const_buffers[uniform.index].MarkAsUsed(static_cast<unsigned>(uniform.index),
                                                      static_cast<unsigned>(uniform.offset), stage);
        std::string value =
            'c' + std::to_string(uniform.index) + '[' + std::to_string(uniform.offset) + ']';

        if (regs[dest_reg].IsFloat()) {
            return value;
        } else if (regs[dest_reg].IsInteger()) {
            return "floatBitsToInt(" + value + ')';
        } else {
            UNREACHABLE();
        }
    }

    /// Add declarations for registers
    void GenerateDeclarations() {
        for (const auto& reg : regs) {
            for (const auto& type : reg.DeclaredTypes()) {
                declarations.AddLine(GLSLRegister::GetTypeString(type) + ' ' +
                                     GLSLRegister::GetPrefixString(type) +
                                     std::to_string(reg.GetIndex()) + " = 0;");
            }
        }
        declarations.AddNewLine();

        for (const auto& index : declr_input_attribute) {
            // TODO(bunnei): Use proper number of elements for these
            declarations.AddLine("layout(location = " +
                                 std::to_string(static_cast<u32>(index) -
                                                static_cast<u32>(Attribute::Index::Attribute_0)) +
                                 ") in vec4 " + GetInputAttribute(index) + ';');
        }
        declarations.AddNewLine();

        for (const auto& index : declr_output_attribute) {
            // TODO(bunnei): Use proper number of elements for these
            declarations.AddLine("layout(location = " +
                                 std::to_string(static_cast<u32>(index) -
                                                static_cast<u32>(Attribute::Index::Attribute_0)) +
                                 ") out vec4 " + GetOutputAttribute(index) + ';');
        }
        declarations.AddNewLine();

        unsigned const_buffer_layout = 0;
        for (const auto& entry : GetConstBuffersDeclarations()) {
            declarations.AddLine("layout(std430) buffer " + entry.GetName());
            declarations.AddLine('{');
            declarations.AddLine("    float c" + std::to_string(entry.GetIndex()) + "[];");
            declarations.AddLine("};");
            declarations.AddNewLine();
            ++const_buffer_layout;
        }
        declarations.AddNewLine();
    }

    /// Returns a list of constant buffer declarations
    std::vector<ConstBufferEntry> GetConstBuffersDeclarations() const {
        std::vector<ConstBufferEntry> result;
        std::copy_if(declr_const_buffers.begin(), declr_const_buffers.end(),
                     std::back_inserter(result), [](const auto& entry) { return entry.IsUsed(); });
        return result;
    }

private:
    /// Build GLSL conversion function, e.g. floatBitsToInt, intBitsToFloat, etc.
    const std::string GetGLSLConversionFunc(GLSLRegister::Type src, GLSLRegister::Type dest) const {
        const std::string src_type = GLSLRegister::GetTypeString(src);
        std::string dest_type = GLSLRegister::GetTypeString(dest);
        dest_type[0] = toupper(dest_type[0]);
        return src_type + "BitsTo" + dest_type;
    }

    /// Generates code representing a temporary (GPR) register.
    std::string GetRegister(const Register& reg, unsigned elem) {
        if (reg == Register::ZeroIndex) {
            return "0";
        }

        return regs[reg.GetSwizzledIndex(elem)].GetActiveString();
    }

    /**
     * Writes code that does a register assignment to value operation.
     * @param reg The destination register to use.
     * @param elem The element to use for the operation.
     * @param value The code representing the value to assign.
     * @param dest_num_components Number of components in the destination.
     * @param value_num_components Number of components in the value.
     * @param is_abs Optional, when True, applies absolute value to output.
     * @param dest_elem Optional, the destination element to use for the operation.
     */
    void SetRegister(const Register& reg, u64 elem, const std::string& value,
                     u64 dest_num_components, u64 value_num_components, bool is_abs,
                     u64 dest_elem) {
        std::string dest = GetRegister(reg, dest_elem);
        if (dest_num_components > 1) {
            dest += GetSwizzle(elem);
        }

        std::string src = '(' + value + ')';
        if (value_num_components > 1) {
            src += GetSwizzle(elem);
        }

        src = is_abs ? "abs(" + src + ')' : src;

        shader.AddLine(dest + " = " + src + ';');
    }

    /// Build the GLSL register list.
    void BuildRegisterList() {
        for (size_t index = 0; index < Register::NumRegisters; ++index) {
            regs.emplace_back(index, shader);
        }
    }

    /// Generates code representing an input attribute register.
    std::string GetInputAttribute(Attribute::Index attribute) {
        switch (attribute) {
        case Attribute::Index::Position:
            return "position";
        case Attribute::Index::TessCoordInstanceIDVertexID:
            // TODO(Subv): Find out what the values are for the first two elements when inside a
            // vertex shader, and what's the value of the fourth element when inside a Tess Eval
            // shader.
            ASSERT(stage == Maxwell3D::Regs::ShaderStage::Vertex);
            return "vec4(0, 0, gl_InstanceID, gl_VertexID)";
        default:
            const u32 index{static_cast<u32>(attribute) -
                            static_cast<u32>(Attribute::Index::Attribute_0)};
            if (attribute >= Attribute::Index::Attribute_0) {
                declr_input_attribute.insert(attribute);
                return "input_attribute_" + std::to_string(index);
            }

            NGLOG_CRITICAL(HW_GPU, "Unhandled input attribute: {}", index);
            UNREACHABLE();
        }
    }

    /// Generates code representing an output attribute register.
    std::string GetOutputAttribute(Attribute::Index attribute) {
        switch (attribute) {
        case Attribute::Index::Position:
            return "position";
        default:
            const u32 index{static_cast<u32>(attribute) -
                            static_cast<u32>(Attribute::Index::Attribute_0)};
            if (attribute >= Attribute::Index::Attribute_0) {
                declr_output_attribute.insert(attribute);
                return "output_attribute_" + std::to_string(index);
            }

            NGLOG_CRITICAL(HW_GPU, "Unhandled output attribute: {}", index);
            UNREACHABLE();
        }
    }

    /// Generates code to use for a swizzle operation.
    static std::string GetSwizzle(u64 elem) {
        ASSERT(elem <= 3);
        std::string swizzle = ".";
        swizzle += "xyzw"[elem];
        return swizzle;
    }

    ShaderWriter& shader;
    ShaderWriter& declarations;
    std::vector<GLSLRegister> regs;
    std::set<Attribute::Index> declr_input_attribute;
    std::set<Attribute::Index> declr_output_attribute;
    std::array<ConstBufferEntry, Maxwell3D::Regs::MaxConstBuffers> declr_const_buffers;
    const Maxwell3D::Regs::ShaderStage& stage;
};

class GLSLGenerator {
public:
    GLSLGenerator(const std::set<Subroutine>& subroutines, const ProgramCode& program_code,
                  u32 main_offset, Maxwell3D::Regs::ShaderStage stage)
        : subroutines(subroutines), program_code(program_code), main_offset(main_offset),
          stage(stage) {

        Generate();
    }

    std::string GetShaderCode() {
        return declarations.GetResult() + shader.GetResult();
    }

    /// Returns entries in the shader that are useful for external functions
    ShaderEntries GetEntries() const {
        return {regs.GetConstBuffersDeclarations()};
    }

private:
    /// Gets the Subroutine object corresponding to the specified address.
    const Subroutine& GetSubroutine(u32 begin, u32 end) const {
        auto iter = subroutines.find(Subroutine{begin, end});
        ASSERT(iter != subroutines.end());
        return *iter;
    }

    /// Generates code representing a 19-bit immediate value
    static std::string GetImmediate19(const Instruction& instr) {
        return std::to_string(instr.alu.GetImm20_19());
    }

    /// Generates code representing a 32-bit immediate value
    static std::string GetImmediate32(const Instruction& instr) {
        return std::to_string(instr.alu.GetImm20_32());
    }

    /// Generates code representing a texture sampler.
    std::string GetSampler(const Sampler& sampler) const {
        // TODO(Subv): Support more than just texture sampler 0
        ASSERT_MSG(sampler.index == Sampler::Index::Sampler_0, "unsupported");
        const unsigned index{static_cast<unsigned>(sampler.index.Value()) -
                             static_cast<unsigned>(Sampler::Index::Sampler_0)};
        return "tex[" + std::to_string(index) + ']';
    }

    /**
     * Adds code that calls a subroutine.
     * @param subroutine the subroutine to call.
     */
    void CallSubroutine(const Subroutine& subroutine) {
        if (subroutine.exit_method == ExitMethod::AlwaysEnd) {
            shader.AddLine(subroutine.GetName() + "();");
            shader.AddLine("return true;");
        } else if (subroutine.exit_method == ExitMethod::Conditional) {
            shader.AddLine("if (" + subroutine.GetName() + "()) { return true; }");
        } else {
            shader.AddLine(subroutine.GetName() + "();");
        }
    }

    /*
     * Writes code that assigns a predicate boolean variable.
     * @param pred The id of the predicate to write to.
     * @param value The expression value to assign to the predicate.
     */
    void SetPredicate(u64 pred, const std::string& value) {
        using Tegra::Shader::Pred;
        // Can't assign to the constant predicate.
        ASSERT(pred != static_cast<u64>(Pred::UnusedIndex));

        std::string variable = 'p' + std::to_string(pred);
        shader.AddLine(variable + " = " + value + ';');
        declr_predicates.insert(std::move(variable));
    }

    /*
     * Returns the condition to use in the 'if' for a predicated instruction.
     * @param instr Instruction to generate the if condition for.
     * @returns string containing the predicate condition.
     */
    std::string GetPredicateCondition(u64 index, bool negate) const {
        using Tegra::Shader::Pred;
        std::string variable;

        // Index 7 is used as an 'Always True' condition.
        if (index == static_cast<u64>(Pred::UnusedIndex))
            variable = "true";
        else
            variable = 'p' + std::to_string(index);

        if (negate) {
            return "!(" + variable + ')';
        }

        return variable;
    }

    /**
     * Returns the comparison string to use to compare two values in the 'set' family of
     * instructions.
     * @params condition The condition used in the 'set'-family instruction.
     * @returns String corresponding to the GLSL operator that matches the desired comparison.
     */
    std::string GetPredicateComparison(Tegra::Shader::PredCondition condition) const {
        using Tegra::Shader::PredCondition;
        static const std::unordered_map<PredCondition, const char*> PredicateComparisonStrings = {
            {PredCondition::LessThan, "<"},      {PredCondition::Equal, "=="},
            {PredCondition::LessEqual, "<="},    {PredCondition::GreaterThan, ">"},
            {PredCondition::GreaterEqual, ">="},
        };

        auto comparison = PredicateComparisonStrings.find(condition);
        ASSERT_MSG(comparison != PredicateComparisonStrings.end(),
                   "Unknown predicate comparison operation");
        return comparison->second;
    }

    /**
     * Returns the operator string to use to combine two predicates in the 'setp' family of
     * instructions.
     * @params operation The operator used in the 'setp'-family instruction.
     * @returns String corresponding to the GLSL operator that matches the desired operator.
     */
    std::string GetPredicateCombiner(Tegra::Shader::PredOperation operation) const {
        using Tegra::Shader::PredOperation;
        static const std::unordered_map<PredOperation, const char*> PredicateOperationStrings = {
            {PredOperation::And, "&&"},
            {PredOperation::Or, "||"},
            {PredOperation::Xor, "^^"},
        };

        auto op = PredicateOperationStrings.find(operation);
        ASSERT_MSG(op != PredicateOperationStrings.end(), "Unknown predicate operation");
        return op->second;
    }

    /*
     * Returns whether the instruction at the specified offset is a 'sched' instruction.
     * Sched instructions always appear before a sequence of 3 instructions.
     */
    bool IsSchedInstruction(u32 offset) const {
        // sched instructions appear once every 4 instructions.
        static constexpr size_t SchedPeriod = 4;
        u32 absolute_offset = offset - main_offset;

        return (absolute_offset % SchedPeriod) == 0;
    }

    /**
     * Compiles a single instruction from Tegra to GLSL.
     * @param offset the offset of the Tegra shader instruction.
     * @return the offset of the next instruction to execute. Usually it is the current offset
     * + 1. If the current instruction always terminates the program, returns PROGRAM_END.
     */
    u32 CompileInstr(u32 offset) {
        // Ignore sched instructions when generating code.
        if (IsSchedInstruction(offset)) {
            return offset + 1;
        }

        const Instruction instr = {program_code[offset]};
        const auto opcode = OpCode::Decode(instr);

        // Decoding failure
        if (!opcode) {
            NGLOG_CRITICAL(HW_GPU, "Unhandled instruction: {0:x}", instr.value);
            UNREACHABLE();
        }

        shader.AddLine("// " + std::to_string(offset) + ": " + opcode->GetName());

        using Tegra::Shader::Pred;
        ASSERT_MSG(instr.pred.full_pred != Pred::NeverExecute,
                   "NeverExecute predicate not implemented");

        if (instr.pred.pred_index != static_cast<u64>(Pred::UnusedIndex)) {
            shader.AddLine("if (" +
                           GetPredicateCondition(instr.pred.pred_index, instr.negate_pred != 0) +
                           ')');
            shader.AddLine('{');
            ++shader.scope;
        }

        switch (opcode->GetType()) {
        case OpCode::Type::Arithmetic: {
            std::string op_a = instr.alu.negate_a ? "-" : "";
            op_a += regs.GetRegisterAsFloat(instr.gpr8);
            if (instr.alu.abs_a) {
                op_a = "abs(" + op_a + ')';
            }

            std::string op_b = instr.alu.negate_b ? "-" : "";

            if (instr.is_b_imm) {
                op_b += GetImmediate19(instr);
            } else {
                if (instr.is_b_gpr) {
                    op_b += regs.GetRegisterAsFloat(instr.gpr20);
                } else {
                    op_b += regs.GetUniform(instr.uniform, instr.gpr0);
                }
            }

            if (instr.alu.abs_b) {
                op_b = "abs(" + op_b + ')';
            }

            switch (opcode->GetId()) {
            case OpCode::Id::MOV_C:
            case OpCode::Id::MOV_R: {
                regs.SetRegisterToFloat(instr.gpr0, 0, op_b, 1, 1);
                break;
            }

            case OpCode::Id::MOV32_IMM: {
                // mov32i doesn't have abs or neg bits.
                regs.SetRegisterToFloat(instr.gpr0, 0, GetImmediate32(instr), 1, 1);
                break;
            }
            case OpCode::Id::FMUL_C:
            case OpCode::Id::FMUL_R:
            case OpCode::Id::FMUL_IMM: {
                regs.SetRegisterToFloat(instr.gpr0, 0, op_a + " * " + op_b, 1, 1, instr.alu.abs_d);
                break;
            }
            case OpCode::Id::FMUL32_IMM: {
                // fmul32i doesn't have abs or neg bits.
                regs.SetRegisterToFloat(
                    instr.gpr0, 0,
                    regs.GetRegisterAsFloat(instr.gpr8) + " * " + GetImmediate32(instr), 1, 1);
                break;
            }
            case OpCode::Id::FADD_C:
            case OpCode::Id::FADD_R:
            case OpCode::Id::FADD_IMM: {
                regs.SetRegisterToFloat(instr.gpr0, 0, op_a + " + " + op_b, 1, 1, instr.alu.abs_d);
                break;
            }
            case OpCode::Id::MUFU: {
                switch (instr.sub_op) {
                case SubOp::Cos:
                    regs.SetRegisterToFloat(instr.gpr0, 0, "cos(" + op_a + ')', 1, 1,
                                            instr.alu.abs_d);
                    break;
                case SubOp::Sin:
                    regs.SetRegisterToFloat(instr.gpr0, 0, "sin(" + op_a + ')', 1, 1,
                                            instr.alu.abs_d);
                    break;
                case SubOp::Ex2:
                    regs.SetRegisterToFloat(instr.gpr0, 0, "exp2(" + op_a + ')', 1, 1,
                                            instr.alu.abs_d);
                    break;
                case SubOp::Lg2:
                    regs.SetRegisterToFloat(instr.gpr0, 0, "log2(" + op_a + ')', 1, 1,
                                            instr.alu.abs_d);
                    break;
                case SubOp::Rcp:
                    regs.SetRegisterToFloat(instr.gpr0, 0, "1.0 / " + op_a, 1, 1, instr.alu.abs_d);
                    break;
                case SubOp::Rsq:
                    regs.SetRegisterToFloat(instr.gpr0, 0, "inversesqrt(" + op_a + ')', 1, 1,
                                            instr.alu.abs_d);
                    break;
                case SubOp::Min:
                    regs.SetRegisterToFloat(instr.gpr0, 0, "min(" + op_a + "," + op_b + ')', 1, 1,
                                            instr.alu.abs_d);
                    break;
                default:
                    NGLOG_CRITICAL(HW_GPU, "Unhandled MUFU sub op: {0:x}",
                                   static_cast<unsigned>(instr.sub_op.Value()));
                    UNREACHABLE();
                }
                break;
            }
            case OpCode::Id::FMNMX_C:
            case OpCode::Id::FMNMX_R:
            case OpCode::Id::FMNMX_IMM: {
                std::string condition =
                    GetPredicateCondition(instr.alu.fmnmx.pred, instr.alu.fmnmx.negate_pred != 0);
                std::string parameters = op_a + ',' + op_b;
                regs.SetRegisterToFloat(instr.gpr0, 0,
                                        '(' + condition + ") ? min(" + parameters + ") : max(" +
                                            parameters + ')',
                                        1, 1);
                break;
            }
            case OpCode::Id::RRO_C:
            case OpCode::Id::RRO_R:
            case OpCode::Id::RRO_IMM: {
                // Currently RRO is only implemented as a register move.
                // Usage of `abs_b` and `negate_b` here should also be correct.
                regs.SetRegisterToFloat(instr.gpr0, 0, op_b, 1, 1);
                NGLOG_WARNING(HW_GPU, "RRO instruction is incomplete");
                break;
            }
            default: {
                NGLOG_CRITICAL(HW_GPU, "Unhandled arithmetic instruction: {}", opcode->GetName());
                UNREACHABLE();
            }
            }
            break;
        }
        case OpCode::Type::Ffma: {
            std::string op_a = regs.GetRegisterAsFloat(instr.gpr8);
            std::string op_b = instr.ffma.negate_b ? "-" : "";
            std::string op_c = instr.ffma.negate_c ? "-" : "";

            switch (opcode->GetId()) {
            case OpCode::Id::FFMA_CR: {
                op_b += regs.GetUniform(instr.uniform, instr.gpr0);
                op_c += regs.GetRegisterAsFloat(instr.gpr39);
                break;
            }
            case OpCode::Id::FFMA_RR: {
                op_b += regs.GetRegisterAsFloat(instr.gpr20);
                op_c += regs.GetRegisterAsFloat(instr.gpr39);
                break;
            }
            case OpCode::Id::FFMA_RC: {
                op_b += regs.GetRegisterAsFloat(instr.gpr39);
                op_c += regs.GetUniform(instr.uniform, instr.gpr0);
                break;
            }
            case OpCode::Id::FFMA_IMM: {
                op_b += GetImmediate19(instr);
                op_c += regs.GetRegisterAsFloat(instr.gpr39);
                break;
            }
            default: {
                NGLOG_CRITICAL(HW_GPU, "Unhandled FFMA instruction: {}", opcode->GetName());
                UNREACHABLE();
            }
            }

            regs.SetRegisterToFloat(instr.gpr0, 0, op_a + " * " + op_b + " + " + op_c, 1, 1);
            break;
        }
        case OpCode::Type::Conversion: {
            ASSERT_MSG(instr.conversion.size == Register::Size::Word, "Unimplemented");
            ASSERT_MSG(!instr.conversion.negate_a, "Unimplemented");
            ASSERT_MSG(!instr.conversion.saturate_a, "Unimplemented");

            switch (opcode->GetId()) {
            case OpCode::Id::I2I_R:
            case OpCode::Id::I2F_R: {
                ASSERT_MSG(!instr.conversion.selector, "Unimplemented");

                std::string op_a =
                    regs.GetRegisterAsInteger(instr.gpr20, 0, instr.conversion.is_signed);

                if (instr.conversion.abs_a) {
                    op_a = "abs(" + op_a + ')';
                }

                regs.SetRegisterToInteger(instr.gpr0, instr.conversion.is_signed, 0, op_a, 1, 1);
                break;
            }
            case OpCode::Id::F2F_R: {
                std::string op_a = regs.GetRegisterAsFloat(instr.gpr20);

                if (instr.conversion.abs_a) {
                    op_a = "abs(" + op_a + ')';
                }

                regs.SetRegisterToFloat(instr.gpr0, 0, op_a, 1, 1);
                break;
            }
            default: {
                NGLOG_CRITICAL(HW_GPU, "Unhandled conversion instruction: {}", opcode->GetName());
                UNREACHABLE();
            }
            }
            break;
        }
        case OpCode::Type::Memory: {
            const Attribute::Index attribute = instr.attribute.fmt20.index;

            switch (opcode->GetId()) {
            case OpCode::Id::LD_A: {
                ASSERT_MSG(instr.attribute.fmt20.size == 0, "untested");
                regs.SetRegisterToInputAttibute(instr.gpr0, instr.attribute.fmt20.element,
                                                attribute);
                break;
            }
            case OpCode::Id::ST_A: {
                ASSERT_MSG(instr.attribute.fmt20.size == 0, "untested");
                regs.SetOutputAttributeToRegister(attribute, instr.attribute.fmt20.element,
                                                  instr.gpr0);
                break;
            }
            case OpCode::Id::TEXS: {
                ASSERT_MSG(instr.attribute.fmt20.size == 4, "untested");
                const std::string op_a = regs.GetRegisterAsFloat(instr.gpr8);
                const std::string op_b = regs.GetRegisterAsFloat(instr.gpr20);
                const std::string sampler = GetSampler(instr.sampler);
                const std::string coord = "vec2 coords = vec2(" + op_a + ", " + op_b + ");";
                // Add an extra scope and declare the texture coords inside to prevent
                // overwriting them in case they are used as outputs of the texs instruction.
                shader.AddLine("{");
                ++shader.scope;
                shader.AddLine(coord);
                const std::string texture = "texture(" + sampler + ", coords)";
                for (unsigned elem = 0; elem < instr.attribute.fmt20.size; ++elem) {
                    regs.SetRegisterToFloat(instr.gpr0, elem, texture, 1, 4, false, elem);
                }
                --shader.scope;
                shader.AddLine("}");
                break;
            }
            default: {
                NGLOG_CRITICAL(HW_GPU, "Unhandled memory instruction: {}", opcode->GetName());
                UNREACHABLE();
            }
            }
            break;
        }
        case OpCode::Type::FloatSetPredicate: {
            std::string op_a = instr.fsetp.neg_a ? "-" : "";
            op_a += regs.GetRegisterAsFloat(instr.gpr8);

            if (instr.fsetp.abs_a) {
                op_a = "abs(" + op_a + ')';
            }

            std::string op_b{};

            if (instr.is_b_imm) {
                if (instr.fsetp.neg_b) {
                    // Only the immediate version of fsetp has a neg_b bit.
                    op_b += '-';
                }
                op_b += '(' + GetImmediate19(instr) + ')';
            } else {
                if (instr.is_b_gpr) {
                    op_b += regs.GetRegisterAsFloat(instr.gpr20);
                } else {
                    op_b += regs.GetUniform(instr.uniform, instr.gpr0);
                }
            }

            if (instr.fsetp.abs_b) {
                op_b = "abs(" + op_b + ')';
            }

            using Tegra::Shader::Pred;
            // We can't use the constant predicate as destination.
            ASSERT(instr.fsetp.pred3 != static_cast<u64>(Pred::UnusedIndex));

            std::string second_pred =
                GetPredicateCondition(instr.fsetp.pred39, instr.fsetp.neg_pred != 0);

            std::string comparator = GetPredicateComparison(instr.fsetp.cond);
            std::string combiner = GetPredicateCombiner(instr.fsetp.op);

            std::string predicate = '(' + op_a + ") " + comparator + " (" + op_b + ')';
            // Set the primary predicate to the result of Predicate OP SecondPredicate
            SetPredicate(instr.fsetp.pred3,
                         '(' + predicate + ") " + combiner + " (" + second_pred + ')');

            if (instr.fsetp.pred0 != static_cast<u64>(Pred::UnusedIndex)) {
                // Set the secondary predicate to the result of !Predicate OP SecondPredicate,
                // if enabled
                SetPredicate(instr.fsetp.pred0,
                             "!(" + predicate + ") " + combiner + " (" + second_pred + ')');
            }
            break;
        }
        case OpCode::Type::FloatSet: {
            std::string op_a = instr.fset.neg_a ? "-" : "";
            op_a += regs.GetRegisterAsFloat(instr.gpr8);

            if (instr.fset.abs_a) {
                op_a = "abs(" + op_a + ')';
            }

            std::string op_b = instr.fset.neg_b ? "-" : "";

            if (instr.is_b_imm) {
                std::string imm = GetImmediate19(instr);
                if (instr.fset.neg_imm)
                    op_b += "(-" + imm + ')';
                else
                    op_b += imm;
            } else {
                if (instr.is_b_gpr) {
                    op_b += regs.GetRegisterAsFloat(instr.gpr20);
                } else {
                    op_b += regs.GetUniform(instr.uniform, instr.gpr0);
                }
            }

            if (instr.fset.abs_b) {
                op_b = "abs(" + op_b + ')';
            }

            // The fset instruction sets a register to 1.0 if the condition is true, and to 0
            // otherwise.
            std::string second_pred =
                GetPredicateCondition(instr.fset.pred39, instr.fset.neg_pred != 0);

            std::string comparator = GetPredicateComparison(instr.fset.cond);
            std::string combiner = GetPredicateCombiner(instr.fset.op);

            std::string predicate = "(((" + op_a + ") " + comparator + " (" + op_b + ")) " +
                                    combiner + " (" + second_pred + "))";

            regs.SetRegisterToFloat(instr.gpr0, 0, predicate + " ? 1.0 : 0.0", 1, 1);
            break;
        }
        default: {
            switch (opcode->GetId()) {
            case OpCode::Id::EXIT: {
                ASSERT_MSG(instr.pred.pred_index == static_cast<u64>(Pred::UnusedIndex),
                           "Predicated exits not implemented");

                // Final color output is currently hardcoded to GPR0-3 for fragment shaders
                if (stage == Maxwell3D::Regs::ShaderStage::Fragment) {
                    shader.AddLine("color.r = " + regs.GetRegisterAsFloat(0) + ';');
                    shader.AddLine("color.g = " + regs.GetRegisterAsFloat(1) + ';');
                    shader.AddLine("color.b = " + regs.GetRegisterAsFloat(2) + ';');
                    shader.AddLine("color.a = " + regs.GetRegisterAsFloat(3) + ';');
                }

                shader.AddLine("return true;");
                offset = PROGRAM_END - 1;
                break;
            }
            case OpCode::Id::KIL: {
                shader.AddLine("discard;");
                break;
            }
            case OpCode::Id::IPA: {
                const auto& attribute = instr.attribute.fmt28;
                regs.SetRegisterToInputAttibute(instr.gpr0, attribute.element, attribute.index);
                break;
            }
            default: {
                NGLOG_CRITICAL(HW_GPU, "Unhandled instruction: {}", opcode->GetName());
                UNREACHABLE();
            }
            }

            break;
        }
        }

        // Close the predicate condition scope.
        if (instr.pred.pred_index != static_cast<u64>(Pred::UnusedIndex)) {
            --shader.scope;
            shader.AddLine('}');
        }

        return offset + 1;
    }

    /**
     * Compiles a range of instructions from Tegra to GLSL.
     * @param begin the offset of the starting instruction.
     * @param end the offset where the compilation should stop (exclusive).
     * @return the offset of the next instruction to compile. PROGRAM_END if the program
     * terminates.
     */
    u32 CompileRange(u32 begin, u32 end) {
        u32 program_counter;
        for (program_counter = begin; program_counter < (begin > end ? PROGRAM_END : end);) {
            program_counter = CompileInstr(program_counter);
        }
        return program_counter;
    }

    void Generate() {
        // Add declarations for all subroutines
        for (const auto& subroutine : subroutines) {
            shader.AddLine("bool " + subroutine.GetName() + "();");
        }
        shader.AddNewLine();

        // Add the main entry point
        shader.AddLine("bool exec_shader() {");
        ++shader.scope;
        CallSubroutine(GetSubroutine(main_offset, PROGRAM_END));
        --shader.scope;
        shader.AddLine("}\n");

        // Add definitions for all subroutines
        for (const auto& subroutine : subroutines) {
            std::set<u32> labels = subroutine.labels;

            shader.AddLine("bool " + subroutine.GetName() + "() {");
            ++shader.scope;

            if (labels.empty()) {
                if (CompileRange(subroutine.begin, subroutine.end) != PROGRAM_END) {
                    shader.AddLine("return false;");
                }
            } else {
                labels.insert(subroutine.begin);
                shader.AddLine("uint jmp_to = " + std::to_string(subroutine.begin) + "u;");
                shader.AddLine("while (true) {");
                ++shader.scope;

                shader.AddLine("switch (jmp_to) {");

                for (auto label : labels) {
                    shader.AddLine("case " + std::to_string(label) + "u: {");
                    ++shader.scope;

                    auto next_it = labels.lower_bound(label + 1);
                    u32 next_label = next_it == labels.end() ? subroutine.end : *next_it;

                    u32 compile_end = CompileRange(label, next_label);
                    if (compile_end > next_label && compile_end != PROGRAM_END) {
                        // This happens only when there is a label inside a IF/LOOP block
                        shader.AddLine("{ jmp_to = " + std::to_string(compile_end) + "u; break; }");
                        labels.emplace(compile_end);
                    }

                    --shader.scope;
                    shader.AddLine('}');
                }

                shader.AddLine("default: return false;");
                shader.AddLine('}');

                --shader.scope;
                shader.AddLine('}');

                shader.AddLine("return false;");
            }

            --shader.scope;
            shader.AddLine("}\n");

            DEBUG_ASSERT(shader.scope == 0);
        }

        GenerateDeclarations();
    }

    /// Add declarations for registers
    void GenerateDeclarations() {
        regs.GenerateDeclarations();

        for (const auto& pred : declr_predicates) {
            declarations.AddLine("bool " + pred + " = false;");
        }
        declarations.AddNewLine();
    }

private:
    const std::set<Subroutine>& subroutines;
    const ProgramCode& program_code;
    const u32 main_offset;
    Maxwell3D::Regs::ShaderStage stage;

    ShaderWriter shader;
    ShaderWriter declarations;
    GLSLRegisterManager regs{shader, declarations, stage};

    // Declarations
    std::set<std::string> declr_predicates;
}; // namespace Decompiler

std::string GetCommonDeclarations() {
    return "bool exec_shader();";
}

boost::optional<ProgramResult> DecompileProgram(const ProgramCode& program_code, u32 main_offset,
                                                Maxwell3D::Regs::ShaderStage stage) {
    try {
        auto subroutines = ControlFlowAnalyzer(program_code, main_offset).GetSubroutines();
        GLSLGenerator generator(subroutines, program_code, main_offset, stage);
        return ProgramResult{generator.GetShaderCode(), generator.GetEntries()};
    } catch (const DecompileFail& exception) {
        NGLOG_ERROR(HW_GPU, "Shader decompilation failed: {}", exception.what());
    }
    return boost::none;
}

} // namespace Decompiler
} // namespace GLShader
