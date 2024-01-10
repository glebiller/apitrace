#pragma once

namespace interceptor {

    struct ModelFingerprint {
        unsigned int iaVertexBufferStride;
        unsigned int iaVertexBufferByteWidth;
        unsigned int iaIndexBufferByteWidth;
        unsigned int psConstantBufferByteWidth;

        bool operator==(const ModelFingerprint& rhs) const {
            return iaVertexBufferStride == rhs.iaVertexBufferStride
                && iaVertexBufferByteWidth == rhs.iaVertexBufferByteWidth
                && iaIndexBufferByteWidth == rhs.iaIndexBufferByteWidth
                && psConstantBufferByteWidth == rhs.psConstantBufferByteWidth;
        }
    };

    constexpr ModelFingerprint TEST = {
        36, 47736, 12936, 256
    };
    constexpr ModelFingerprint JAINA = {
        36, 267408, 65280, 256
    };
}

