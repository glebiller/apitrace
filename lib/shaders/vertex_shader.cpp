#include "vertex_shader.hpp"

namespace shaders {
    void VertexShader::parseShaderFile(std::istream& shaderStream) {
        std::string line;
        while (std::getline(shaderStream, line)) {
            // Skip comment lines and empty lines
            if (line.empty() || line[0] == '/' || line[0] == ' ') {
                continue;
            }

            if (line.rfind("dcl_output_siv ", 0) == 0 && line.find(" position") != std::string::npos) {
                positionOutputIndex = line[16] - '0';
            }

            const size_t firstSpacePos = line.find(' ');
            const size_t modifierPos = line.find('_');
            const size_t operationEnd = std::min(modifierPos, firstSpacePos);
            const OPERATION operation = stringToOperation(line.substr(0, operationEnd));
            OPERATION_MODIFIER modifier = NOOP;
            if (operation != UNKNOWN_OP && firstSpacePos != operationEnd) {
                const char modifierChar = line[modifierPos + 1];
                if (modifierChar == 's') {
                    modifier = SAT;
                }
            }
            const std::string parameters = line.substr(firstSpacePos + 1);

            if (operation != UNKNOWN_OP) {
                instructions.push_back(Instruction{operation, modifier, parameters});
            }
        }
    }
}