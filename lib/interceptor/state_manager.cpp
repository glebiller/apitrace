//
// Created by Kissy on 12/30/2023.
//

#include "state_manager.hpp"

#include <DirectXMath.h>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <os.hpp>

#include "debugger.hpp"

namespace interceptor {

    static DirectX::XMMATRIX identityMatrix = DirectX::XMMatrixIdentity();

    StateManager::StateManager() = default;

    StateManager::~StateManager() = default;

    void StateManager::Shutdown() {
        SetWindowLongPtr(context.window, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(context.originalWndProc));
        debugger.Shutdown();
    }

    LRESULT __stdcall StateManager::InterceptWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CLOSE:
                stateManager.Shutdown();
            case WM_INPUT:
                return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }

        if (Debugger::WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }
        return CallWindowProc(stateManager.context.originalWndProc, hWnd, uMsg, wParam, lParam);
    }

    void StateManager::ReportResourceUpdated(ID3D11Resource* pResource, const void* ptr, size_t size) {
        if (pResource == currentVSConstantBuffer) {
            ReportVSConstantBuffersUpdated(ptr, size);
        }
    }

    void StateManager::ReportBeforePresent() {
        debugger.EndFrame();
    }

    void StateManager::ReportAfterPresent() {
        currentExecution.frame++;
        currentExecution.drawCall = 0;
        debugger.NewFrame();
    }

    WNDPROC originalWndProc;

    void StateManager::SetContext(IDXGISwapChain* pSwapChain,  const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
        ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext) {
        context.originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(pSwapChainDesc->OutputWindow, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(InterceptWndProc)));
        context.window = pSwapChainDesc->OutputWindow;
        context.pSwapChain = pSwapChain;
        context.pDevice = pDevice;
        context.pImmediateContext = pImmediateContext;

        debugger.Init(context.window, context.pDevice, context.pImmediateContext);
    }

    void StateManager::ReportDraw(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
        currentExecution.drawCall++;

        debugger.ReportDraw(IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    void StateManager::ReportIASetVertexBuffers(UINT stride, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers) {
        if (ppVertexBuffers && NumBuffers) {
            if (ID3D11Buffer* pVertexBuffers = ppVertexBuffers[0]) {
                pVertexBuffers->GetDesc(&tmpBufferDesc);
                currentModel.iaVertexBufferStride = stride;
                currentModel.iaVertexBufferByteWidth = tmpBufferDesc.ByteWidth;
            }
        }
    }

    void StateManager::ReportIASetIndexBuffer(ID3D11Buffer* pIndexBuffer) {
        if (pIndexBuffer) {
            pIndexBuffer->GetDesc(&tmpBufferDesc);
            currentModel.iaIndexBufferByteWidth = tmpBufferDesc.ByteWidth;
        }
    }

    void StateManager::ReportAfterVSSetConstantBuffers(UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
        if (ppConstantBuffers && NumBuffers) {
            if (ID3D11Buffer* pConstantBuffers = ppConstantBuffers[0]) {
                currentVSConstantBuffer = pConstantBuffers;
                pConstantBuffers->GetDesc(&tmpBufferDesc);
                currentModel.vsConstantBufferByteWidth = tmpBufferDesc.ByteWidth;
            }
        }
    }

    void StateManager::ReportAfterPSSetConstantBuffers(UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
        if (ppConstantBuffers && NumBuffers) {
            if (ID3D11Buffer* pConstantBuffers = ppConstantBuffers[0]) {
                pConstantBuffers->GetDesc(&tmpBufferDesc);
                currentModel.psConstantBufferByteWidth = tmpBufferDesc.ByteWidth;
            }
        }
    }

    void StateManager::ReportVSConstantBuffersUpdated(const void* ptr, size_t size) {
        constexpr int sizeOfOneRow = 4 * sizeof(float);
        const auto pData = static_cast<const float*>(ptr);
        std::memcpy(&viewMatrix, pData, 3 * sizeOfOneRow);
        if (size >= 3584) {
            std::memcpy(&projectionMatrix, &pData[197 * 4], 4 * sizeOfOneRow);
        } else {
            projectionMatrix = DirectX::XMMatrixIdentity();
        }
    }

    void StateManager::ReportViewport(UINT NumViewports, const D3D11_VIEWPORT* pViewports) {
        if (NumViewports) {
            currentViewport.width = pViewports->Width;
            currentViewport.height = pViewports->Height;
            currentViewport.topLeftX = pViewports->TopLeftX;
            currentViewport.topLeftY = pViewports->TopLeftY;
        }
    }

    void StateManager::ReportMap(ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags,
        D3D11_MAPPED_SUBRESOURCE* pMappedResource) {
    }

    void StateManager::ReportBeforeUnmap(ID3D11Resource* pResource, UINT Subresource) {
    }

    void StateManager::ReportAfterUnmap(ID3D11Resource* pResource, UINT Subresource) {
    }

    bool StateManager::ShouldShadowMap(const ID3D11Resource* pResource, const UINT Subresource) const {
        return pResource == currentVSConstantBuffer && Subresource == 0;
    }

    DirectX::XMFLOAT2 StateManager::GetCurrentDrawPosition() const {
        const DirectX::XMVECTOR v = DirectX::XMVectorSet(0, 0, 0, 1);
        DirectX::XMVECTOR screeSpace = DirectX::XMVectorSet(0, 0, 0, 0);
        if (&projectionMatrix == &identityMatrix) {
            screeSpace = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatX(v), viewMatrix.r[0], screeSpace);
            screeSpace = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatY(v), viewMatrix.r[1], screeSpace);
            screeSpace = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatZ(v), viewMatrix.r[2], screeSpace);
            screeSpace = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatW(v), viewMatrix.r[3], screeSpace);
        } else {
            screeSpace = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatX(DirectX::XMVector4Dot(viewMatrix.r[0], v)), projectionMatrix.r[0], screeSpace);
            screeSpace = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatY(DirectX::XMVector4Dot(viewMatrix.r[1], v)), projectionMatrix.r[1], screeSpace);
            screeSpace = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatZ(DirectX::XMVector4Dot(viewMatrix.r[2], v)), projectionMatrix.r[2], screeSpace);
            screeSpace = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatW(v), projectionMatrix.r[3], screeSpace);
        }

        return DirectX::XMFLOAT2(
             currentViewport.topLeftX + (screeSpace.m128_f32[0] / screeSpace.m128_f32[3] * (currentViewport.width / 2.0f) + currentViewport.width / 2.0f),
            currentViewport.topLeftY + (currentViewport.height / 2.0f - screeSpace.m128_f32[1] / screeSpace.m128_f32[3] * (currentViewport.height / 2.0f))
            );
    }

    StateManager stateManager;
}
