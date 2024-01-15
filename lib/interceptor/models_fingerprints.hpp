#pragma once
#include <iosfwd>
#include <sstream>

#include "json/json.hpp"

namespace interceptor {
    struct ModelFingerprint {
        unsigned int iaVertexBufferStride;
        unsigned int iaVertexBufferByteWidth;
        unsigned int iaIndexBufferByteWidth;
        unsigned int vsConstantBufferByteWidth;
        unsigned int psConstantBufferByteWidth;

        bool operator==(const ModelFingerprint& rhs) const {
            return iaVertexBufferStride == rhs.iaVertexBufferStride
                && iaVertexBufferByteWidth == rhs.iaVertexBufferByteWidth
                && iaIndexBufferByteWidth == rhs.iaIndexBufferByteWidth
                && vsConstantBufferByteWidth == rhs.vsConstantBufferByteWidth
                && psConstantBufferByteWidth == rhs.psConstantBufferByteWidth;
        }
        bool operator<(const ModelFingerprint& other) const {
            return std::tie(iaVertexBufferByteWidth, iaVertexBufferStride, iaIndexBufferByteWidth,
                            vsConstantBufferByteWidth, psConstantBufferByteWidth) <
                   std::tie(other.iaVertexBufferByteWidth, other.iaVertexBufferStride, other.iaIndexBufferByteWidth,
                           other.vsConstantBufferByteWidth, other.psConstantBufferByteWidth);
        }

        [[nodiscard]] std::string toString() const {
            std::ostringstream name;
            name << iaVertexBufferStride << "," << iaVertexBufferByteWidth << ","
                << iaIndexBufferByteWidth << "," << vsConstantBufferByteWidth << "," << psConstantBufferByteWidth;
            return name.str();
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(ModelFingerprint,
            iaVertexBufferStride, iaVertexBufferByteWidth, iaIndexBufferByteWidth,
            vsConstantBufferByteWidth, psConstantBufferByteWidth)
    };

    constexpr ModelFingerprint TEST = {
        36, 47736, 12936, 256
    };
    constexpr ModelFingerprint WHITEMANE = {
        32, 261760, 71322, 256
    };
}

