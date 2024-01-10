#pragma once

#include "vertex_shader.hpp"

namespace shaders {
    struct Vector4fWithModifier {
        const DirectX::XMVECTOR& modifier;
        const DirectX::XMVECTOR& src;
    };

    class VertexShaderSimulator {
    public:
        void process(const VertexShader& shader);

    private:
        std::array<DirectX::XMVECTOR, 16> inputs{};
        DirectX::XMVECTOR output{};
        std::array<std::array<DirectX::XMVECTOR, 512>, 15> constants{};
        std::array<DirectX::XMVECTOR, 32> temps{};

        int getConstantComponentIndex(const std::unique_ptr<InstructionComponent>& constantComponent) const;

        DirectX::XMVECTOR& getDst(const Instruction& instruction, int i);
        const DirectX::XMVECTOR& getSrc(const Instruction& instruction, int i) const;

        static const DirectX::XMVECTOR& getSrcModifier(const Instruction& instruction, int i);

        void mov(const Instruction& instruction);
        void dp2(const Instruction& instruction);
        void dp3(const Instruction& instruction);
        void dp4(const Instruction& instruction);
        void sqrt(const Instruction& instruction);
        void mul(const Instruction& instruction);
        void imul(const Instruction& instruction);
        void ftoi(const Instruction& instruction);
        void mad(const Instruction& instruction);
        void rsq(const Instruction& instruction);
        void div(const Instruction& instruction);
        void exp(const Instruction& instruction);
        void add(const Instruction& instruction);
        void sincos(const Instruction& instruction);
    };
}
