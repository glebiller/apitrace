#pragma once

#include "instruction_components.hpp"

#include <vector>

namespace shaders {

    enum OPERATION_MODIFIER {
        NOOP,
        SAT,
    };

    struct Instruction {
        OPERATION operation = UNKNOWN_OP;
        OPERATION_MODIFIER modifier = NOOP;
        std::vector<InstructionComponent> comps{};

        Instruction(const OPERATION operation, const OPERATION_MODIFIER modifier, const std::string& parameters)
            : operation(operation), modifier(modifier) {
            size_t start = 0;
            while (start < parameters.length()) {
                size_t separatorPos = parameters.find(", ", start);

                if (separatorPos == std::string::npos) {
                    comps.emplace_back(parameters.substr(start));
                    break;
                }

                const size_t openParenthesisPos = parameters.find('(', start);
                if (openParenthesisPos != std::string::npos && openParenthesisPos < separatorPos) {
                    separatorPos = parameters.find(", ", parameters.find(')', openParenthesisPos));
                    if (separatorPos == std::string::npos) {
                        comps.emplace_back(parameters.substr(start));
                        break;
                    }
                }

                comps.emplace_back(parameters.substr(start, separatorPos - start));
                start = separatorPos + 2; // Skip past ", "
            }
        }

        [[nodiscard]] std::string toString() const {
            std::ostringstream oss;
            oss << operationToString(operation);
            if (modifier == SAT) {
                oss << "_sat";
            }
            oss << " ";
            for (const auto& comp : comps) {
                oss << ", " << comp.toString();
            }

            return oss.str();
        }
    };

}