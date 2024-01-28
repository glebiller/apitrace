#include "model_fingerprint.hpp"

#include <functional>
#include <json/json.hpp>

#include "types.hpp"

namespace interceptor {
    ModelFingerprint::ModelFingerprint() = default;

    template <std::size_t N>
    unsigned CountBuffers(const std::array<ID3D11Buffer*, N>& array) {
        for (int count = 0; count < array.size(); count++) {
            if (array[count] == nullptr) {
                return count;
            }
        }
        return 0;
    }

    ModelFingerprint::ModelFingerprint(const unsigned int id,
                                       ID3D11Buffer* IAIndexBuffer,
                                       const DXGI_FORMAT IAIndexBufferFormat,
                                       const ID3D11InputLayout* IAInputLayout,
                                       const UINT IAVertexBufferStrides,
                                       const std::array<ID3D11Buffer*, 32>& IAVertexBuffers,
                                       const std::array<ID3D11Buffer*, 15>& VSConstantBuffers,
                                       const std::array<ID3D11Buffer*, 15>& PSConstantBuffers,
                                       std::unordered_map<const ID3D11Buffer*, D3D11_BUFFER_DESC>& bufferDescs)
        : id(id),
          iaIndexBufferFormat(IAIndexBufferFormat),
          iaInputLayout(IAInputLayout),
          iaVertexBufferStrides(IAVertexBufferStrides) {
        this->iaIndexBufferByteWidth = GetCachedBufferDesc(IAIndexBuffer, bufferDescs).ByteWidth;
        this->iaVertexBuffersCount = CountBuffers(IAVertexBuffers);
        this->iaVertexBufferByteWidth = GetCachedBufferDesc(IAVertexBuffers[0], bufferDescs).ByteWidth;
        this->vsConstantBuffersCount = CountBuffers(VSConstantBuffers);
        this->vsConstantBufferByteWidth = GetCachedBufferDesc(VSConstantBuffers[0], bufferDescs).ByteWidth;
        this->psConstantBuffersCount = CountBuffers(PSConstantBuffers);
        this->psConstantBufferByteWidth = GetCachedBufferDesc(PSConstantBuffers[0], bufferDescs).ByteWidth;

        hash = 17;

        hash = hash * 31 + std::hash<unsigned int>()(iaIndexBufferByteWidth);
        hash = hash * 31 + std::hash<unsigned int>()(iaIndexBufferFormat);
        hash = hash * 31 + std::hash<unsigned int>()(iaVertexBufferStrides);
        hash = hash * 31 + std::hash<unsigned int>()(iaVertexBufferByteWidth);
        hash = hash * 31 + std::hash<unsigned int>()(vsConstantBufferByteWidth);
        hash = hash * 31 + std::hash<unsigned int>()(psConstantBufferByteWidth);
    }

    ModelFingerprint::ModelFingerprint(const ModelFingerprintHash hash) : hash(hash) {}

    bool ModelFingerprint::operator<(const ModelFingerprint& other) const {
        return hash < other.hash;
    }

    D3D11_BUFFER_DESC& ModelFingerprint::GetCachedBufferDesc(ID3D11Buffer* buffer,
                                                             std::unordered_map<const ID3D11Buffer*, D3D11_BUFFER_DESC>&
                                                             bufferDescs) {
        if (buffer == nullptr) {
            return EMPTY_BUFFER_DESC;
        }

        auto& indexBufferDesc = bufferDescs[buffer];
        if (indexBufferDesc == EMPTY_BUFFER_DESC) {
            buffer->GetDesc(&indexBufferDesc);
        }
        return indexBufferDesc;
    }
}
