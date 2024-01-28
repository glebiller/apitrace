#include "model_instance.hpp"

#include "imgui.h"
#include "state_manager.hpp"

namespace interceptor {
    ModelDraw::ModelDraw(const ModelFingerprintHash modelFingerprintHash,
                         const ModelInstanceHash modelInstanceHash,
                         const unsigned drawCall) :
        modelFingerprintHash(modelFingerprintHash),
        modelInstanceHash(modelInstanceHash),
        drawCall(drawCall) {}

    void ModelDraw::Update() {
        const auto& modelFingerprintsInstances = stateManager.GetModelFingerprintsInstances();
        const auto foundModelFingerprintInstances = modelFingerprintsInstances.find(modelFingerprintHash);
        if (foundModelFingerprintInstances == modelFingerprintsInstances.end()) {
            return;
        }

        const auto& modelFingerprintInstances = foundModelFingerprintInstances->second;
        const auto foundModelFingerprintInstance = modelFingerprintInstances.find(modelInstanceHash);
        if (foundModelFingerprintInstance == modelFingerprintInstances.end()) {
            return;
        }

        const auto& modelInstance = foundModelFingerprintInstance->second;
        const auto data = reinterpret_cast<const float*>(vertexConstantBuffers[0].data());

        // Update Matrices
        constexpr int sizeOfOneRow = 4 * sizeof(float);
        std::memcpy(&worldTransform, data + (modelInstance.viewTransformIndex * 3 * 4), 4 * sizeOfOneRow);
        if (vertexConstantBuffers[0].size() >= 3584) {
            worldTransform.r[3] = DirectX::XMVectorSet(0, 0, 0, 1);
            worldTransform = XMMatrixTranspose(worldTransform);
            std::memcpy(&viewTransform, data + 197 * 4, 4 * sizeOfOneRow);
            // 207 == camera translation
        } else {
            viewTransform = DirectX::XMMatrixIdentity();
        }

        const auto modelCenterVector = XMLoadFloat3(&modelInstance.modelCenter);
        projectedModelCenter = GetScreenPosition(modelInstance.viewport, modelCenterVector);

        // Bounding Box
        const auto adjustedModelCenter = DirectX::XMVectorAdd(modelCenterVector, modelInstance.modelCenterOffset);
        const auto scaledBoundingBox = DirectX::XMVectorScale(DirectX::XMVectorMultiply(modelInstance.boundingBoxSize, modelInstance.boundingBoxScale), 0.5);
        const auto boundingBoxX = DirectX::XMVectorGetX(scaledBoundingBox);
        const auto boundingBoxY = DirectX::XMVectorGetY(scaledBoundingBox);
        const auto boundingBoxZ = DirectX::XMVectorGetZ(scaledBoundingBox);
        for (int i = 0; i < 8; ++i) {
            DirectX::XMVECTOR boundingBoxCorner = DirectX::XMVectorSet(
                (i & 1) ? boundingBoxX : -boundingBoxX,
                (i & 2) ? boundingBoxY : -boundingBoxY,
                (i & 4) ? boundingBoxZ : -boundingBoxZ,
                1.0f
            );
            boundingBoxCorner = DirectX::XMVectorAdd(boundingBoxCorner, adjustedModelCenter);
            const auto projectedCorner = GetScreenPosition(modelInstance.viewport, boundingBoxCorner);
            projectedBoundingBoxCorners[i] = projectedCorner;
        }
        // Faces center
        for (int i = 0; i < 3; ++i) { // Iterate over x, y, z axes
            for (int sign = -1; sign <= 1; sign += 2) { // Toggle between -1 and 1
                DirectX::XMVECTOR center = DirectX::XMVectorZero();
                // Set the corresponding component based on the current axis and sign
                switch (i) {
                case 0: // x-axis
                    center = DirectX::XMVectorSetX(center, boundingBoxX * sign);
                    break;
                case 1: // y-axis
                    center = DirectX::XMVectorSetY(center, boundingBoxY * sign);
                    break;
                default:
                case 2: // z-axis
                    center = DirectX::XMVectorSetZ(center, boundingBoxZ * sign);
                    break;
                }

                center = DirectX::XMVectorAdd(center, adjustedModelCenter);
                const auto projectedCenter = GetScreenPosition(modelInstance.viewport, center);
                projectedFacesCenters[i * 2 + (sign + 1) / 2] = projectedCenter;
            }
        }

        screenModelBoundingBox[0] = DirectX::XMFLOAT2{ FLT_MAX, FLT_MAX };
        screenModelBoundingBox[1] = DirectX::XMFLOAT2{ FLT_MIN, FLT_MIN };
        for (int i = 0; i < 6; i++) {
            screenModelBoundingBox[0].x = std::min(projectedFacesCenters[i].x, screenModelBoundingBox[0].x);
            screenModelBoundingBox[0].y = std::min(projectedFacesCenters[i].y, screenModelBoundingBox[0].y);
            screenModelBoundingBox[1].x = std::max(projectedFacesCenters[i].x, screenModelBoundingBox[1].x);
            screenModelBoundingBox[1].y = std::max(projectedFacesCenters[i].y, screenModelBoundingBox[1].y);
        }
    }

    const DirectX::XMFLOAT2& ModelDraw::GetScreenPosition(const Viewport& viewport,
                                                          const DirectX::XMVECTOR& point) const {
        DirectX::XMVECTOR ndcSpace = XMVector3Transform(point, worldTransform);
        ndcSpace = XMVector3Transform(ndcSpace, viewTransform);

        // TODO pre-compute viewport center & half width?
        const float x = viewport.topLeftX + viewport.width / 2.0f +
            ndcSpace.m128_f32[0] / ndcSpace.m128_f32[3] * (viewport.width / 2.0f);
        const float y = viewport.topLeftY + viewport.height / 2.0f +
            - ndcSpace.m128_f32[1] / ndcSpace.m128_f32[3] * (viewport.height / 2.0f);
        return DirectX::XMFLOAT2(x, y);
    }

    ModelInstance::ModelInstance() = default;

    ModelInstance::ModelInstance(const ModelFingerprintHash modelFingerprintHash, const unsigned int id,
                                 const UINT IndexCount, const UINT BaseVertexLocation,
                                 const UINT StartIndexLocation, const UINT IAVertexBufferOffsets,
                                 const std::array<ID3D11ShaderResourceView*, 128>& PsShaderResourceViews,
                                 const Viewport& viewport)
        : modelFingerprintHash(modelFingerprintHash),
          id(id),
          IndexCount(IndexCount),
          StartIndexLocation(StartIndexLocation),
          BaseVertexLocation(BaseVertexLocation),
          IAVertexBufferOffsets(IAVertexBufferOffsets),
          viewport(viewport) {
        // TODO handle multiple dimensions
        if (PsShaderResourceViews[0] != nullptr) {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            PsShaderResourceViews[0]->GetDesc(&desc);
            this->psShaderResourceNumElements = desc.Buffer.NumElements;
            this->psShaderResourceWidth = desc.Buffer.ElementWidth;
            this->psShaderResourceDimension = desc.ViewDimension;
        }

        UpdateHash();
    }

    void ModelInstance::ResetBoundingBoxSize() {
        DirectX::XMFLOAT3 minVertex(FLT_MAX, FLT_MAX, FLT_MAX);
        DirectX::XMFLOAT3 maxVertex(FLT_MIN, FLT_MIN, FLT_MIN);
        IterateVertices([this, &minVertex, &maxVertex](const float* vertex) {
            this->modelCenter.x += vertex[0];
            this->modelCenter.y += vertex[1];
            this->modelCenter.z += vertex[2];

            minVertex.x = std::min(minVertex.x, vertex[0]);
            minVertex.y = std::min(minVertex.y, vertex[1]);
            minVertex.z = std::min(minVertex.z, vertex[2]);
            maxVertex.x = std::max(maxVertex.x, vertex[0]);
            maxVertex.y = std::max(maxVertex.y, vertex[1]);
            maxVertex.z = std::max(maxVertex.z, vertex[2]);
        });

        modelCenter.x /= static_cast<float>(IndexCount);
        modelCenter.y /= static_cast<float>(IndexCount);
        modelCenter.z /= static_cast<float>(IndexCount);

        boundingBoxSize.m128_f32[0] = maxVertex.x - minVertex.x;
        boundingBoxSize.m128_f32[1] = maxVertex.y - minVertex.y;
        boundingBoxSize.m128_f32[2] = maxVertex.z - minVertex.z;
        boundingBoxSize.m128_f32[3] = 1;
    }

    void ModelInstance::IterateVertices(const std::function<void(const float*)>& handler) const {
        const auto& modelFingerprints = stateManager.GetModelFingerprints();
        const auto foundModelFingerprint = modelFingerprints.find(modelFingerprintHash);
        if (foundModelFingerprint == modelFingerprints.end()) {
            return;
        }
        const auto& modelFingerprint = foundModelFingerprint->second;
        const void* indexData = modelFingerprint.indexBuffer.data();

        for (size_t i = StartIndexLocation; i < StartIndexLocation + IndexCount; i++) {
            uint32_t index;
            switch (modelFingerprint.iaIndexBufferFormat) {
            case DXGI_FORMAT_R16_UINT:
                index = static_cast<const uint16_t*>(indexData)[i];
                break;
            default:
                index = static_cast<const uint32_t*>(indexData)[i];
            }

            const size_t byteOffset = index * modelFingerprint.iaVertexBufferStrides + IAVertexBufferOffsets;
            if (byteOffset < modelFingerprint.vertexBuffers[0].size()) {
                const auto vertex = reinterpret_cast<const float*>(modelFingerprint.vertexBuffers[0].data() + byteOffset);
                handler(vertex);
            } else {
                // TODO handle multiple vertexBuffers
            }
        }
    }

    void ModelInstance::UpdateHash() {
        hash = 17;

        hash = hash * 31 + std::hash<unsigned int>()(IndexCount);
        //hash = hash * 31 + std::hash<unsigned int>()(StartIndexLocation); Changing too often for 2d models
        //hash = hash * 31 + std::hash<unsigned int>()(IAVertexBufferOffsets); Changing too often for 2d models
        hash = hash * 31 + std::hash<unsigned int>()(BaseVertexLocation);
        hash = hash * 31 + std::hash<unsigned int>()(psShaderResourceNumElements);
        hash = hash * 31 + std::hash<unsigned int>()(psShaderResourceWidth);
        hash = hash * 31 + std::hash<unsigned int>()(psShaderResourceDimension);
        hash = hash * 31 + std::hash<float>()(viewport.width);
        hash = hash * 31 + std::hash<float>()(viewport.height);
        hash = hash * 31 + std::hash<float>()(viewport.topLeftX);
        hash = hash * 31 + std::hash<float>()(viewport.topLeftY);
    }
}
