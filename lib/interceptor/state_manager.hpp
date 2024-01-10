//
// Created by Kissy on 12/30/2023.
//

#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <dxgi.h>

#include "models_fingerprints.hpp"

namespace interceptor {

    struct Context {
        HWND window;
        WNDPROC originalWndProc;
        IDXGISwapChain* pSwapChain;
        ID3D11Device* pDevice;
        ID3D11DeviceContext* pImmediateContext;
    };

    struct ExecutionInfo {
        int frame;
        int drawCall;
    };

    class StateManager {
        Context context{nullptr, nullptr, nullptr, nullptr, nullptr};
        ExecutionInfo currentExecution{0, 0};
        ModelFingerprint currentModel{0, 0, 0, 0 };
        ID3D11Buffer* currentVSConstantBuffer = nullptr;
        D3D11_BUFFER_DESC tmpBufferDesc{};
        DirectX::XMMATRIX viewMatrix{};
        DirectX::XMMATRIX projectionMatrix{};
        float viewportHeight{}, viewportWidth{};

    public:
        /**
         * Should never called directly -- use stateManager singleton below
         * instead.
         */
        StateManager();
        ~StateManager();

        static LRESULT InterceptWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        const Context& Context() const {
            return context;
        }

        const ExecutionInfo& CurrentExecution() const {
            return currentExecution;
        }

        int CurrentFrame() const {
            return currentExecution.frame;
        }

        const ModelFingerprint& CurrentModel() const {
            return currentModel;
        }

        void ReportResourceUpdated(ID3D11Resource* pResource, const void* ptr, size_t size);

        void ReportBeforePresent();

        void ReportAfterPresent();

        void SetContext(IDXGISwapChain* idxgi_swap_chain, const DXGI_SWAP_CHAIN_DESC* dxgi_swap_chain_desc,
                        ID3D11Device* pp_device, ID3D11DeviceContext* id_3d_11device_context);

        void ReportDraw(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

        void ReportIASetVertexBuffers(UINT stride, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers);

        void ReportIASetIndexBuffer(ID3D11Buffer* pIndexBuffer);

        void ReportAfterVSSetConstantBuffers(UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);

        void ReportAfterPSSetConstantBuffers(UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);

        void ReportVSConstantBuffersUpdated(const void *ptr, size_t size);

        void ReportViewport(UINT NumViewports, const D3D11_VIEWPORT* pViewports);

        void ReportMap(ID3D11Resource * pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE * pMappedResource);

        void ReportBeforeUnmap(ID3D11Resource * pResource, UINT Subresource);
        void ReportAfterUnmap(ID3D11Resource * pResource, UINT Subresource);

        bool ShouldShadowMap(const ID3D11Resource * pResource, UINT Subresource) const;

        DirectX::XMFLOAT2 GetCurrentDrawPosition() const;
    };

    /**
     * Singleton.
     */
    extern StateManager stateManager;

}
