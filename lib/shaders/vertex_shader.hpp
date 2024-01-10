# pragma once

#include "instruction.hpp"

namespace shaders {
    class VertexShader {
        int positionOutputIndex;
        std::vector<Instruction> instructions{};

        // Method to parse the shader file
        void parseShaderFile(std::istream& shaderStream);

    public:
        explicit VertexShader(std::istream& shaderStream) {
            parseShaderFile(shaderStream);
        }

        [[nodiscard]] const std::vector<Instruction>& getInstructions() const {
            return instructions;
        }

        [[nodiscard]] std::string toString() const {
            std::ostringstream oss;
            for (const auto& instruction: instructions) {
                oss << instruction.toString() << std::endl;
            }
            return oss.str();
        }
    };
}
