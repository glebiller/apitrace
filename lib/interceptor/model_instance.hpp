#pragma once
#include <ostream>
#include <DirectXMath.h>
#include <json/json.hpp>

#include "model_fingerprint.hpp"
#include "types.hpp"

NLOHMANN_JSON_NAMESPACE_BEGIN
template <>
struct adl_serializer<DirectX::XMVECTOR> {
    static DirectX::XMVECTOR from_json(const json& j) {
        return DirectX::XMVectorSet(j[0], j[1], j[2], j[3]);
    }

    static void to_json(json& j, const DirectX::XMVECTOR& vector) {
        j = vector.m128_f32;
    }
};
NLOHMANN_JSON_NAMESPACE_END

namespace interceptor {
    using ModelInstanceHash = std::size_t;

    class ModelDraw {
    public:
        ModelFingerprintHash modelFingerprintHash = 0;
        ModelInstanceHash modelInstanceHash = 0;
        unsigned int drawCall = 0;
        // Only if tracked
        std::vector<std::vector<BYTE>> vertexConstantBuffers;
        std::vector<std::vector<BYTE>> pixelConstantBuffers;
        DirectX::XMMATRIX worldTransform = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX viewTransform = DirectX::XMMatrixIdentity();

        DirectX::XMFLOAT2 projectedModelCenter{};
        std::array<DirectX::XMFLOAT2, 6> projectedFacesCenters{};
        std::array<DirectX::XMFLOAT2, 8> projectedBoundingBoxCorners{};
        std::array<DirectX::XMFLOAT2, 2> screenModelBoundingBox{};

        ModelDraw(ModelFingerprintHash modelFingerprintHash, ModelInstanceHash modelInstanceHash, 
            unsigned int drawCall);

        void Update();

    private:
        const DirectX::XMFLOAT2& GetScreenPosition(const Viewport& viewport, const DirectX::XMVECTOR& point) const;
    };

    class ModelInstance {
    public: // TODO not public?
        ModelFingerprintHash modelFingerprintHash = 0;

        unsigned int id = 0;
        ModelInstanceHash hash = 0;

        // Index
        unsigned int IndexCount = 0;
        unsigned int StartIndexLocation = 0;

        // Vertex
        unsigned int BaseVertexLocation = 0;
        unsigned int IAVertexBufferOffsets = 0;

        // Pixel
        unsigned int psShaderResourceNumElements = 0;
        unsigned int psShaderResourceWidth = 0;
        D3D11_SRV_DIMENSION psShaderResourceDimension;

        // Viewport
        Viewport viewport{};

        // State
        bool tracked = false;
        bool hidden = false;
        unsigned int viewTransformIndex = 0;
        DirectX::XMFLOAT3 modelCenter{};
        DirectX::XMVECTOR modelCenterOffset{};
        DirectX::XMVECTOR boundingBoxSize{};
        DirectX::XMVECTOR boundingBoxScale = DirectX::XMVectorSplatOne();

        // Only if tracked
        std::vector<ModelDraw> draws;

        ModelInstance();
        ModelInstance(ModelFingerprintHash modelFingerprintHash, unsigned int id,
                      UINT IndexCount, UINT BaseVertexLocation, UINT StartIndexLocation, UINT IAVertexBufferOffsets,
                      const std::array<ID3D11ShaderResourceView*, 128>& PsShaderResourceViews,
                      const Viewport& viewport);

        void ResetBoundingBoxSize();

        void IterateVertices(const std::function<void(const float*)>& handler) const;

        bool operator==(const ModelInstance& other) const {
            return hash == other.hash;
        }

        bool operator<(const ModelInstance& other) const {
            return hash < other.hash;
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ModelInstance,
            modelFingerprintHash,
            id, hash,
            IndexCount, StartIndexLocation,
            BaseVertexLocation, IAVertexBufferOffsets,
            psShaderResourceNumElements, psShaderResourceWidth, psShaderResourceDimension,
            viewport, 
            tracked,
            viewTransformIndex,
            modelCenterOffset,
            boundingBoxSize
        )

    private:
        void UpdateHash();
    };
}
