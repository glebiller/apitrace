#pragma once

#include <array>
#include <DirectXMath.h>
#include <iomanip>
#include <string>
#include <sstream>

#include "operations.hpp"

namespace shaders {

    enum SIGN {
        PLUS = 0,
        MINUS = 1
    };

    enum BUFFER_TYPE {
        BUFFER_UNKNOWN = 0,
        CONSTANT_BUFFER = 1,
        INPUT_BUFFER = 2,
        OUTPUT_BUFFER = 3,
        TEMPORARY_REGISTRY = 4,
        CONSTANT_VALUE = 5,
        CONSTANT_NUMBER = 6,
    };

    inline int swizzleChatToIndex(const char ch) {
        switch (ch) {
            default:
            case 'x':
                return 0;
            case'y':
                return 1;
            case 'z':
                return 2;
            case 'w':
                return 3;
        }
    }

    inline char swizzleToChar(int index) {
        switch (index) {
            default:
            case 0:
                return 'x';
            case 1:
                return 'y';
            case 2:
                return 'z';
            case 3:
                return 'w';
        }
    }

    struct InstructionComponent {
        SIGN sign = PLUS; // TODO abs modifier
        BUFFER_TYPE type = BUFFER_UNKNOWN;
        int index = -1;
        int swizzleSize = 0;
        std::array<int, 4> swizzle{};
        int constantSize = 0;
        DirectX::XMVECTOR constantValues{};
        std::unique_ptr<InstructionComponent> constantComponent{};
        int offset = 0;

        explicit InstructionComponent(const std::string& component) {
            if (component == "null") {
                type = BUFFER_UNKNOWN;
                return;
            }
            size_t start = 0;
            if (component[start] == '-') {
                sign = MINUS;
                ++start;
            }

            if (component[start] == 'r') {
                type = TEMPORARY_REGISTRY;
                parseRegistryOrBuffer(component, start);
            } else if (component[start] == 'l') {
                type = CONSTANT_VALUE;
                parseConstant(component, start);
            } else if (component[start] == 'v') {
                type = INPUT_BUFFER;
                parseRegistryOrBuffer(component, start);
            } else if (component[start] == 'o') {
                type = OUTPUT_BUFFER;
                parseRegistryOrBuffer(component, start);
            } else if (component[start] == 'c') {
                type = CONSTANT_BUFFER;
                parseConstantBuffer(component, start);
            } else { // Constant number
                type = CONSTANT_NUMBER;
                parseConstantValue(component, start);
            }
        }

        [[nodiscard]] std::string toString() const {
            std::ostringstream oss;
            if (type == UNKNOWN_OP) {
                oss << "null";
            } else {
                if (sign == MINUS) {
                    oss << "-";
                }
                oss << typeToString(type);
                if (type == CONSTANT_VALUE) {
                    oss << "(";
                    for (const auto& value : constantValues.m128_f32) {
                        oss << std::setprecision(7) << value << ", ";
                    }
                    oss << ")";
                } else {
                    oss << index;
                }
                if (type == CONSTANT_BUFFER && constantComponent) {
                    oss << "[" << constantComponent->toString() << "]";
                }
                if (!swizzle.empty()) {
                    oss << ".";
                    for (const auto index : swizzle) {
                        oss << swizzleToChar(index);
                    }
                }
                if (offset) {
                    oss << " + " << offset;
                }
            }
            return oss.str();
        }

    private:
        void parseSwizzle(const std::string& swizzleStr) {
            swizzleSize = static_cast<int>(swizzleStr.size());
            for (int i = 0; i < swizzleSize; i++) {
                swizzle[i] = swizzleChatToIndex(swizzleStr[i]);
            }
        }

        void parseRegistryOrBuffer(const std::string& comp, const size_t start) { // NOLINT(*-convert-member-functions-to-static)
            // Find the end of the index part (this might include an arithmetic expression)
            size_t end = comp.size();
            const size_t offsetStart = comp.find('+', start);
            if (offsetStart != std::string::npos) {
                offset = std::stoi(comp.substr(offsetStart + 2));
                end = offsetStart - 1;
            }

            // Extract and convert the index
            const size_t swizzleStart = comp.find('.', start);
            if (swizzleStart != std::string::npos) {
                parseSwizzle(comp.substr(swizzleStart + 1, end - swizzleStart - 1));
                end = swizzleStart;
            }
            index = std::stoi(comp.substr(start + 1, end - start - 1));
        }

        void parseConstant(const std::string& comp, const size_t start) { // NOLINT(*-convert-member-functions-to-static)
            const size_t end = comp.find(')', start);
            const std::string values = comp.substr(start + 2, end - start - 2); // Skip "l(" and ")"
            std::istringstream iss(values);
            float value;
            int currentConstant = 0;
            while (iss >> value) {
                constantValues.m128_f32[currentConstant] = value;
                currentConstant++;
                if (iss.peek() == ',') {
                    iss.ignore();
                }
            }
            constantSize = currentConstant;
        }

        void parseConstantBuffer(const std::string& comp, const size_t start) { // NOLINT(*-convert-member-functions-to-static)
            const size_t bracketStart = comp.find('[');
            const size_t bracketEnd = comp.find(']', bracketStart);

            if (bracketStart != std::string::npos && bracketEnd != std::string::npos) {
                index = std::stoi(comp.substr(start + 2, bracketStart - 1));

                std::string constantComponentStr = comp.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
                constantComponent = std::make_unique<InstructionComponent>(constantComponentStr);

                const size_t dotPosition = comp.find('.', bracketEnd);
                if (dotPosition != std::string::npos) {
                    parseSwizzle(comp.substr(dotPosition + 1));
                }
            }
        }

        void parseConstantValue(const std::string& comp, const size_t start) {
            index = std::stoi(comp);
        }

        static std::string typeToString(const BUFFER_TYPE type) {
            switch (type) {
                case CONSTANT_BUFFER:
                    return "cb";
                case INPUT_BUFFER:
                    return "v";
                case OUTPUT_BUFFER:
                    return "o";
                case TEMPORARY_REGISTRY:
                    return "r";
                case CONSTANT_VALUE:
                    return "l";
                case CONSTANT_NUMBER:
                    return "";
                default:
                    return "UNKNOWN";
            }
        }

    };

}
