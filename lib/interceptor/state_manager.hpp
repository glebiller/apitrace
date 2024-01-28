//
// Created by Kissy on 12/30/2023.
//

#pragma once

#include <array>
#include <d3d11.h>
#include <dxgi.h>
#include <unordered_map>

#include "model_fingerprint.hpp"
#include "model_instance.hpp"
#include "types.hpp"

namespace interceptor {
    // TODO remove
    struct InstanceState {
        DirectX::XMVECTOR originOffset{};
        DirectX::XMVECTOR boundingBoxSize{};
    };

    using ModelFingerprintsMap = std::unordered_map<ModelFingerprintHash, ModelFingerprint>;
    using ModelInstancesMap = std::unordered_map<ModelInstanceHash, ModelInstance>;
    using ModelFingerprintsInstancesMap = std::unordered_map<ModelFingerprintHash, ModelInstancesMap>;
    using InputLayoutsMap = std::unordered_map<const ID3D11InputLayout*, std::vector<D3D11_INPUT_ELEMENT_DESC>>;

    struct DirectXContext {
        HWND window;
        WNDPROC originalWndProc;
        IDXGISwapChain* pSwapChain;
        ID3D11Device* pDevice;
        ID3D11DeviceContext* pImmediateContext;
    };

    class StateManager {
        // TODO do not use struct?
        DirectXContext directXContext{nullptr, nullptr, nullptr, nullptr, nullptr};
        ExecutionInfo currentExecution{0, 0};

        ID3D11InputLayout* currentIAInputLayout = nullptr;
        UINT currentIAVertexBufferStrides = 0;
        UINT currentIAVertexBufferOffsets = 0;
        DXGI_FORMAT currentIAIndexBufferFormat = DXGI_FORMAT_UNKNOWN;
        UINT currentIAIndexBufferOffset = 0;
        ID3D11Buffer* currentIAIndexBuffer = nullptr;
        std::array<ID3D11Buffer*, 32> currentIAVertexBuffers = {};
        std::array<ID3D11Buffer*, 15> currentVSConstantBuffers = {};
        std::array<ID3D11Buffer*, 15> currentPSConstantBuffers = {};
        std::array<ID3D11ShaderResourceView*, 128> currentPsShaderResourceViews = {};
        Viewport currentViewport{ 0, 0, 0, 0 };

        ModelFingerprintsMap modelFingerprints;
        ModelFingerprintsInstancesMap modelFingerprintsInstances;

        std::unordered_map<const ID3D11Buffer*, D3D11_BUFFER_DESC> bufferDescs;
        InputLayoutsMap inputLayouts;

    public:
        /**
         * Should never called directly -- use stateManager singleton below
         * instead.
         */
        StateManager();
        ~StateManager();

        static LRESULT InterceptWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void Shutdown();

        void ReportBeforePresent();

        void ReportAfterPresent();

        void SetContext(IDXGISwapChain* pSwapChain, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
                        ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext);

        void ReportAfterCreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, const ID3D11InputLayout* pInputLayout);

        void ReportIASetInputLayout(ID3D11InputLayout* pInputLayout);

        void ReportIASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers,
                                      UINT strides, UINT offsets);

        void ReportIASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset);

        void ReportAfterVSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);

        void ReportAfterPSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);

        void ReportAfterPSSetShaderResources(UINT StartSlot, UINT NumViews,
                                             ID3D11ShaderResourceView* const* ppShaderResourceViews);

        void ReportAfterRSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports);

        bool ReportBeforeDrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

        void ClearModelFingerprints();

        void ClearModelFingerprintsInstances();

        void Import();

        void Export();

        ModelFingerprintsMap& GetModelFingerprints() {
            return modelFingerprints;
        }

        ModelFingerprintsInstancesMap& GetModelFingerprintsInstances() {
            return modelFingerprintsInstances;
        }

        InputLayoutsMap GetInputLayouts() {
            return inputLayouts;
        }
    };

    /**
     * Singleton.
     */
    extern StateManager stateManager;
}
