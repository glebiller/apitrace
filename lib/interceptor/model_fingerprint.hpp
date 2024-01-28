#pragma once

#include <array>
#include <cstddef>
#include <d3d11.h>
#include <string>
#include <unordered_map>

#include "json/json.hpp"

namespace interceptor {
    using ModelFingerprintHash = std::size_t;

    class ModelFingerprint {
    public:
        unsigned int id = 0;
        ModelFingerprintHash hash = 0;

        // Index
        unsigned int iaIndexBufferByteWidth = 0; // Hashed
        DXGI_FORMAT iaIndexBufferFormat = DXGI_FORMAT_UNKNOWN; // Hashed

        // Vertex
        const ID3D11InputLayout* iaInputLayout = nullptr;
        unsigned int iaVertexBuffersCount = 0;
        unsigned int iaVertexBufferStrides = 0; // Hashed
        unsigned int iaVertexBufferByteWidth = 0; // Hashed

        // VS Constant
        unsigned int vsConstantBuffersCount = 0;
        unsigned int vsConstantBufferByteWidth = 0; // Hashed

        // Pixel might not be needed ?
        unsigned int psConstantBuffersCount = 0;
        unsigned int psConstantBufferByteWidth = 0; // Hashed

        // Draw
        unsigned int drawCount = 0;

        // State
        std::string name;
        bool tracked = false;

        // Only if tracked
        std::vector<BYTE> indexBuffer;
        std::vector<std::vector<BYTE>> vertexBuffers;

        ModelFingerprint();
        ModelFingerprint(unsigned int id,
                         ID3D11Buffer* IAIndexBuffer, DXGI_FORMAT IAIndexBufferFormat, 
                         const ID3D11InputLayout* IAInputLayout,
                         UINT IAVertexBufferStrides, const std::array<ID3D11Buffer*, 32>& IAVertexBuffers,
                         const std::array<ID3D11Buffer*, 15>& VSConstantBuffers,
                         const std::array<ID3D11Buffer*, 15>& PSConstantBuffers,
                         std::unordered_map<const ID3D11Buffer*, D3D11_BUFFER_DESC>& bufferDescs);
        explicit ModelFingerprint(ModelFingerprintHash hash);

        bool operator==(const ModelFingerprint& other) const {
            return hash == other.hash;
        }

        bool operator<(const ModelFingerprint& other) const;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ModelFingerprint, 
            id, hash, 
            iaIndexBufferByteWidth, iaIndexBufferFormat,
            iaVertexBuffersCount, iaVertexBufferStrides, iaVertexBufferByteWidth,
            vsConstantBuffersCount, vsConstantBufferByteWidth,
            psConstantBuffersCount, psConstantBufferByteWidth, 
            name, tracked
        )

    private:
        static D3D11_BUFFER_DESC& GetCachedBufferDesc(ID3D11Buffer* buffer,
                                                      std::unordered_map<const ID3D11Buffer*, D3D11_BUFFER_DESC>& bufferDescs);
    };

    struct ModelFingerprintToHash {
        std::size_t operator()(const ModelFingerprint& other) const {
            return other.hash;
        }
    };

    struct ModelFingerprintEqual {
        bool operator()(const ModelFingerprint& obj1, const ModelFingerprint& obj2) const {
            return obj1 == obj2;
        }
    };
}
