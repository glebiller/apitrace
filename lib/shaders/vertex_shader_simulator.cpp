#include "vertex_shader_simulator.hpp"

#include <algorithm>
#include <math.h>

#include "instruction.hpp"

namespace shaders {

    const DirectX::XMVECTOR NEGATIVE = DirectX::XMVectorSet(-1, -1, -1, -1);
    const DirectX::XMVECTOR IDENTITY = DirectX::XMVectorSet(1, 1, 1, 1);

    void VertexShaderSimulator::process(const VertexShader& shader) {
        for (const auto& instruction : shader.getInstructions()) {
            switch (instruction.operation) {
                default:
                case UNKNOWN_OP:
                    break;
                case MOV:

                    break;
                case ADD:
                    break;
                case MAD:
                    break;
                case MUL:
                    break;
                case DP2:
                    break;
                case DP3:
                    break;
                case DP4:
                    break;
                case SQRT:
                    break;
                case FTOI:
                    break;
                case IMUL:
                    break;
                case RSQ:
                    break;
                case DIV:
                    break;
                case EXP:
                    break;
                case SINCOS:
                    break;
            }
        }
    }

    int VertexShaderSimulator::getConstantComponentIndex(const std::unique_ptr<InstructionComponent>& constantComponent) const {
        if (constantComponent->type == CONSTANT_NUMBER) {
            return constantComponent->index;
        }
        // TOOD assert (constantComponent->type == TEMPORARY_REGISTRY)
        const int constantSwizzle = constantComponent->swizzle[0];
        const auto constantIndexRegistry = temps[constantComponent->index].m128_f32;
        return static_cast<int>(constantIndexRegistry[constantSwizzle]) + constantComponent->offset;
    }

    DirectX::XMVECTOR& VertexShaderSimulator::getDst(const Instruction& instruction, const int i) {
        const auto& dst = instruction.comps[i];
        if (dst.type == TEMPORARY_REGISTRY) {
            return temps[dst.index];
        }
        // TODO assert (dst.type == OUTPUT_BUFFER)
        return output;
    }

    const DirectX::XMVECTOR& VertexShaderSimulator::getSrc(const Instruction& instruction, const int i) const {
        const auto& src = instruction.comps[i];
        if (src.type == TEMPORARY_REGISTRY) {
            return temps[src.index];
        }
        if (src.type == INPUT_BUFFER) {
            return inputs[src.index];
        }
        if (src.type == CONSTANT_BUFFER) {
            const int index = getConstantComponentIndex(src.constantComponent);
            return constants[src.index][index];
        }
        // TODO assert (src.type == CONSTANT_VALUE)
        return src.constantValues;
    }

    const DirectX::XMVECTOR& VertexShaderSimulator::getSrcModifier(const Instruction& instruction, const int i) {
        if (const auto& src = instruction.comps[i]; src.sign == MINUS) {
            return NEGATIVE;
        }
        return IDENTITY;
    }

    inline void VertexShaderSimulator::mov(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Modifiers = getSrcModifier(instruction, 1).m128_f32;
        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const size_t dstSwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dstSwizzleSize; i++) {
            const int dst_swizzle = dst0Swizzle[i];
            const int src_swizzle = src0Swizzle[i];
            dst0Components[dst_swizzle] = src0Modifiers[src_swizzle] * src0Components[src_swizzle];
        }
    }

    inline void VertexShaderSimulator::dp2(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const auto& src1Components = getSrc(instruction, 2).m128_f32;
        const auto& src1Swizzle = instruction.comps[2].swizzle;

        dst0Components[dst0Swizzle[0]] = src0Components[src0Swizzle[0]] * src1Components[src1Swizzle[0]] +
                                       src0Components[src0Swizzle[1]] * src1Components[src1Swizzle[1]];
    }

    inline void VertexShaderSimulator::dp3(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const auto& src1Components = getSrc(instruction, 2).m128_f32;
        const auto& src1Swizzle = instruction.comps[2].swizzle;

        dst0Components[dst0Swizzle[0]] = src0Components[src0Swizzle[0]] * src1Components[src1Swizzle[0]] +
                                       src0Components[src0Swizzle[1]] * src1Components[src1Swizzle[1]] +
                                       src0Components[src0Swizzle[2]] * src1Components[src1Swizzle[2]];
    }

    inline void VertexShaderSimulator::dp4(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const auto& src1Components = getSrc(instruction, 2).m128_f32;
        const auto& src1Swizzle = instruction.comps[2].swizzle;

        dst0Components[dst0Swizzle[0]] = src0Components[src0Swizzle[0]] * src1Components[src1Swizzle[0]] +
                                        src0Components[src0Swizzle[1]] * src1Components[src1Swizzle[1]] +
                                        src0Components[src0Swizzle[2]] * src1Components[src1Swizzle[2]] +
                                        src0Components[src0Swizzle[3]] * src1Components[src1Swizzle[3]];
    }

    inline void VertexShaderSimulator::sqrt(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        dst0Components[dst0Swizzle[0]] = std::sqrt(src0Components[src0Swizzle[0]]);
    }

    inline void VertexShaderSimulator::mul(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const auto& src1Components = getSrc(instruction, 2).m128_f32;
        const auto& src1Swizzle = instruction.comps[2].swizzle;

        const size_t dst0SwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dst0SwizzleSize; i++) {
            dst0Components[dst0Swizzle[i]] = src0Components[src0Swizzle[i]] * src1Components[src1Swizzle[i]];
        }
    }

    void VertexShaderSimulator::imul(const Instruction& instruction) {
        auto multiply = [](const float a, const float b, float& hi, float& low) {
            const long long result = static_cast<long long>(a) * static_cast<long long>(b);
            hi = static_cast<int>(result >> 32); // Higher 32 bits NOLINT(*-narrowing-conversions)
            low = static_cast<int>(result & 0xFFFFFFFF); // Lower 32 bits NOLINT(*-narrowing-conversions)
        };

        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        auto& dst1Components = getDst(instruction, 1).m128_f32;
        const auto& dst1Swizzle = instruction.comps[1].swizzle;

        const auto& src0Components = getSrc(instruction, 2).m128_f32;
        const auto& src0Swizzle = instruction.comps[2].swizzle;

        const auto& src1Components = getSrc(instruction, 3).m128_f32;
        const auto& src1Swizzle = instruction.comps[3].swizzle;


        const size_t dst1SwizzleSize = instruction.comps[1].swizzleSize;
        for (int i = 0; i < dst1SwizzleSize; i++) {
            multiply(src0Components[src0Swizzle[i]], src1Components[src1Swizzle[i]], dst0Components[dst0Swizzle[i]], dst1Components[dst1Swizzle[i]]);
        }
    }

    void VertexShaderSimulator::ftoi(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const size_t dst0SwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dst0SwizzleSize; i++) {
            dst0Components[dst0Swizzle[i]] = static_cast<int>(src0Components[src0Swizzle[i]]); // NOLINT(*-narrowing-conversions)
        }
    }

    void VertexShaderSimulator::mad(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const auto& src1Components = getSrc(instruction, 2).m128_f32;
        const auto& src1Swizzle = instruction.comps[2].swizzle;

        const auto& src2Components = getSrc(instruction, 3).m128_f32;
        const auto& src2Swizzle = instruction.comps[3].swizzle;

        const size_t dst0SwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dst0SwizzleSize; i++) {
            dst0Components[dst0Swizzle[i]] = src0Components[src0Swizzle[i]] * src1Components[src1Swizzle[i]] + src2Components[src2Swizzle[i]];
        }
    }

    void VertexShaderSimulator::rsq(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const size_t dst0SwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dst0SwizzleSize; i++) {
            const auto& srcVal = src0Components[src0Swizzle[i]];
            if (srcVal == 0.0f) {
                dst0Components[dst0Swizzle[i]] = FLT_MAX;
            } else {
                dst0Components[dst0Swizzle[i]] = 1.0f / sqrtf(fabsf(srcVal));
            }
        }
    }

    void VertexShaderSimulator::div(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const auto& src1Components = getSrc(instruction, 2).m128_f32;
        const auto& src1Swizzle = instruction.comps[2].swizzle;

        const size_t dst0SwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dst0SwizzleSize; i++) {
            if (instruction.modifier == SAT) {
                dst0Components[dst0Swizzle[i]] = std::clamp(src0Components[src0Swizzle[i]] / src1Components[src1Swizzle[i]], 0.0f, 1.0f);
            } else {
                dst0Components[dst0Swizzle[i]] = src0Components[src0Swizzle[i]] / src1Components[src1Swizzle[i]];
            }
        }
    }

    void VertexShaderSimulator::exp(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const size_t dst0SwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dst0SwizzleSize; i++) {
            dst0Components[dst0Swizzle[i]] = powf(2.0f, src0Components[src0Swizzle[i]]);
        }
    }

    void VertexShaderSimulator::add(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        const auto& src0Components = getSrc(instruction, 1).m128_f32;
        const auto& src0Swizzle = instruction.comps[1].swizzle;

        const auto& src1Components = getSrc(instruction, 2).m128_f32;
        const auto& src1Swizzle = instruction.comps[2].swizzle;

        const size_t dst0SwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dst0SwizzleSize; i++) {
            dst0Components[dst0Swizzle[i]] = src0Components[src0Swizzle[i]] + src1Components[src1Swizzle[i]];
        }
    }

    void VertexShaderSimulator::sincos(const Instruction& instruction) {
        auto& dst0Components = getDst(instruction, 0).m128_f32;
        const auto& dst0Swizzle = instruction.comps[0].swizzle;

        auto& dst1Components = getDst(instruction, 1).m128_f32;
        const auto& dst1Swizzle = instruction.comps[1].swizzle;

        const auto& src0Components = getSrc(instruction, 2).m128_f32;
        const auto& src0Swizzle = instruction.comps[2].swizzle;

        const size_t dst0SwizzleSize = instruction.comps[0].swizzleSize;
        for (int i = 0; i < dst0SwizzleSize; i++) {
            dst0Components[dst1Swizzle[i]] = sinf(src0Components[src0Swizzle[i]]);
            dst1Components[dst0Swizzle[i]] = cosf(src0Components[src0Swizzle[i]]);
        }
    }
}
