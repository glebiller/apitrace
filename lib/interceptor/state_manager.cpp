//
// Created by Kissy on 12/30/2023.
//

#include "state_manager.hpp"

#include <DirectXMath.h>
#include <fstream>
#include <imgui.h>
#include <imgui/backends/imgui_impl_dx11.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <ranges>

#include "helpers.hpp"

namespace {
    std::string modelFingerprintsJsonFilePath = "model-fingerprints.json";
    std::string modelFingerprintsInstancesJsonFilePath = "model-fingerprints-instances.json";
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace interceptor {
    StateManager::StateManager() = default;

    StateManager::~StateManager() = default;

    LRESULT __stdcall StateManager::InterceptWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_CLOSE:
            stateManager.Shutdown();
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        case WM_INPUT:
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }

        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)
            || ImGui::GetIO().WantCaptureMouse
            || ImGui::GetIO().WantCaptureKeyboard) {
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }

        return CallWindowProc(stateManager.directXContext.originalWndProc, hWnd, uMsg, wParam, lParam);
    }

    void StateManager::Shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        SetWindowLongPtr(directXContext.window, GWLP_WNDPROC,
                         reinterpret_cast<LONG_PTR>(directXContext.originalWndProc));
    }

    void StateManager::ReportBeforePresent() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void StateManager::ReportAfterPresent() {
        currentExecution.frame++;
        currentExecution.drawCall = 0;

        for (auto& modelFingerprint : modelFingerprints | std::views::values) {
            modelFingerprint.drawCount = 0;
        }

        // TODO maybe remove too old instances?
        for (auto& modelFingerprintInstances : modelFingerprintsInstances | std::views::values) {
            for (auto& modelInstance : modelFingerprintInstances | std::views::values) {
                modelInstance.draws.clear();
            }
        }
    }

    WNDPROC originalWndProc;

    void StateManager::SetContext(IDXGISwapChain* pSwapChain, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
                                  ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext) {
        directXContext.originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
            pSwapChainDesc->OutputWindow, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(InterceptWndProc)));
        directXContext.window = pSwapChainDesc->OutputWindow;
        directXContext.pSwapChain = pSwapChain;
        directXContext.pDevice = pDevice;
        directXContext.pImmediateContext = pImmediateContext;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplWin32_Init(pSwapChainDesc->OutputWindow);
        ImGui_ImplDX11_Init(pDevice, pImmediateContext);
    }

    void StateManager::ReportAfterCreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
                                                    const UINT NumElements,
                                                    const void* pShaderBytecodeWithInputSignature,
                                                    SIZE_T BytecodeLength,
                                                    const ID3D11InputLayout* pInputLayout) {
        auto& inputLayout = inputLayouts[pInputLayout];
        inputLayout.clear();
        for (UINT i = 0; i < NumElements; ++i) {
            inputLayout.push_back(pInputElementDescs[i]);
        }
    }

    void StateManager::ReportIASetInputLayout(ID3D11InputLayout* pInputLayout) {
        currentIAInputLayout = pInputLayout;
    }

    void StateManager::ReportIASetVertexBuffers(const UINT StartSlot, const UINT NumBuffers,
                                                ID3D11Buffer* const* ppVertexBuffers, const UINT strides,
                                                const UINT offsets) {
        currentIAVertexBufferStrides = strides;
        currentIAVertexBufferOffsets = offsets;

        auto it = currentIAVertexBuffers.begin();
        std::advance(it, StartSlot);
        std::copy_n(ppVertexBuffers, NumBuffers, it);
    }

    void StateManager::ReportIASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset) {
        currentIAIndexBuffer = pIndexBuffer;
        currentIAIndexBufferFormat = Format;
        currentIAIndexBufferOffset = Offset;
    }

    void StateManager::ReportAfterVSSetConstantBuffers(const UINT StartSlot, const UINT NumBuffers,
                                                       ID3D11Buffer* const* ppConstantBuffers) {
        auto it = currentVSConstantBuffers.begin();
        std::advance(it, StartSlot);
        std::copy_n(ppConstantBuffers, NumBuffers, it);
    }

    void StateManager::ReportAfterPSSetConstantBuffers(const UINT StartSlot, const UINT NumBuffers,
                                                       ID3D11Buffer* const* ppConstantBuffers) {
        auto it = currentPSConstantBuffers.begin();
        std::advance(it, StartSlot);
        std::copy_n(ppConstantBuffers, NumBuffers, it);
    }

    void StateManager::ReportAfterPSSetShaderResources(const UINT StartSlot, const UINT NumViews,
                                                       ID3D11ShaderResourceView* const* ppShaderResourceViews) {
        auto it = currentPsShaderResourceViews.begin();
        std::advance(it, StartSlot);
        std::copy_n(ppShaderResourceViews, NumViews, it);
    }

    void StateManager::ReportAfterRSSetViewports(const UINT NumViewports, const D3D11_VIEWPORT* pViewports) {
        if (NumViewports) {
            // TODO handle multiple viewports?
            currentViewport.width = pViewports->Width;
            currentViewport.height = pViewports->Height;
            currentViewport.topLeftX = pViewports->TopLeftX;
            currentViewport.topLeftY = pViewports->TopLeftY;
        }
    }

    bool StateManager::ReportBeforeDrawIndexed(const UINT IndexCount, const UINT StartIndexLocation,
                                               const INT BaseVertexLocation) {
        currentExecution.drawCall++;

        // Fingerprint
        ModelFingerprint drawnModelFingerprint(
            modelFingerprints.size(),
            currentIAIndexBuffer,
            currentIAIndexBufferFormat,
            currentIAInputLayout,
            currentIAVertexBufferStrides,
            currentIAVertexBuffers,
            currentVSConstantBuffers,
            currentPSConstantBuffers,
            bufferDescs
        );
        if (!modelFingerprints.contains(drawnModelFingerprint.hash)) {
            modelFingerprints.insert({drawnModelFingerprint.hash, drawnModelFingerprint});
        }
        auto& modelFingerprint = modelFingerprints[drawnModelFingerprint.hash];
        modelFingerprint.drawCount++;

        if (modelFingerprint.tracked) {
            if (modelFingerprint.indexBuffer.empty()) {
                getBufferContent(directXContext.pDevice, directXContext.pImmediateContext,
                    currentIAIndexBuffer, &modelFingerprint.indexBuffer);
            }
            //if (modelFingerprint.vertexBuffers.empty()) {
                getBuffersContent(directXContext.pDevice, directXContext.pImmediateContext,
                                  currentIAVertexBuffers, modelFingerprint.iaVertexBuffersCount,
                                  &modelFingerprint.vertexBuffers);
            //}
        }

        // Instance
        auto& modelInstances = modelFingerprintsInstances[drawnModelFingerprint.hash];
        auto drawnModelInstance = ModelInstance(
            drawnModelFingerprint.hash,
            modelInstances.size(),
            IndexCount,
            BaseVertexLocation,
            StartIndexLocation,
            currentIAVertexBufferOffsets,
            currentPsShaderResourceViews,
            currentViewport
        );
        if (!modelInstances.contains(drawnModelInstance.hash)) {
            modelInstances.insert({drawnModelInstance.hash, drawnModelInstance});
        }

        auto& modelInstance = modelInstances[drawnModelInstance.hash];
        modelInstance.StartIndexLocation = StartIndexLocation;
        modelInstance.IAVertexBufferOffsets = currentIAVertexBufferOffsets;

        if (modelInstance.tracked) {
            if (DirectX::XMVector3Equal(modelInstance.boundingBoxSize, DirectX::XMVectorZero())) {
                // Compute default boundingBoxSize
                modelInstance.ResetBoundingBoxSize();
            }
        }

        // Draw
        ModelDraw modelDraw{
            drawnModelFingerprint.hash,
            drawnModelInstance.hash,
            currentExecution.drawCall,
        };

        if (modelFingerprint.tracked && modelInstance.tracked) {
            getBuffersContent(directXContext.pDevice, directXContext.pImmediateContext,
                              currentVSConstantBuffers, modelFingerprint.vsConstantBuffersCount,
                              &modelDraw.vertexConstantBuffers);
            getBuffersContent(directXContext.pDevice, directXContext.pImmediateContext,
                              currentPSConstantBuffers, modelFingerprint.psConstantBuffersCount,
                              &modelDraw.pixelConstantBuffers);
            modelDraw.Update();
        }

        modelInstance.draws.push_back(modelDraw);

        // TODO TEMP

        if (currentExecution.frame == 222 && currentExecution.drawCall == 55) {
            /*{
                std::ofstream outFile("C:/Users/guill/Workspace/apitrace/log/sprite.index.bin.txt", std::ios::binary);
                outFile.write(reinterpret_cast<const char*>(modelFingerprint.indexBuffer.data()), modelFingerprint.indexBuffer.size());
                outFile.close();
            }
            {
                std::ofstream outFile("C:/Users/guill/Workspace/apitrace/log/sprite.vertex.0.bin.txt", std::ios::binary);
                outFile.write(reinterpret_cast<const char*>(modelFingerprint.vertexBuffers[0].data()), modelFingerprint.vertexBuffers[0].size());
                outFile.close();
            }
            {
                std::ofstream outFile("C:/Users/guill/Workspace/apitrace/log/sprite.pixel-constant.0.bin.txt", std::ios::binary);
                outFile.write(reinterpret_cast<const char*>(modelDraw.pixelConstantBuffers[0].data()), modelDraw.pixelConstantBuffers[0].size());
                outFile.close();
            }
            {
                std::ofstream outFile("C:/Users/guill/Workspace/apitrace/log/sprite.vertex-constant.0.bin.txt", std::ios::binary);
                outFile.write(reinterpret_cast<const char*>(modelDraw.vertexConstantBuffers[0].data()), modelDraw.vertexConstantBuffers[0].size());
                outFile.close();
            }
            {
                std::ofstream outFile("C:/Users/guill/Workspace/apitrace/log/sprite.model-fingerprint.json.txt", std::ios::binary);
                const nlohmann::json modelFingerprintJson = modelFingerprint;
                outFile << modelFingerprintJson;
                outFile.close();
            }
            {
                std::ofstream outFile("C:/Users/guill/Workspace/apitrace/log/sprite.model-instance.json.txt", std::ios::binary);
                const nlohmann::json modelInstanceJson = modelInstance;
                outFile << modelInstanceJson;
                outFile.close();
            }*/
            
            return false;
        }

        // TODO TEMP

        return !modelInstance.hidden;
    }

    void StateManager::ClearModelFingerprints() {
        // Only non Tracked
        auto it = modelFingerprints.begin();
        while (it != modelFingerprints.end()) {
            if (!it->second.tracked) {
                modelFingerprintsInstances.erase(it->first);
                it = modelFingerprints.erase(it);
            } else {
                ++it;
            }
        }
    }

    void StateManager::ClearModelFingerprintsInstances() {
        for (auto modelFingerprintInstances : modelFingerprintsInstances | std::views::values) {
            // Only non Tracked
            auto it = modelFingerprintInstances.begin();
            while (it != modelFingerprintInstances.end()) {
                if (!it->second.tracked) {
                    it = modelFingerprintInstances.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void StateManager::Import() {
        if (std::ifstream file(modelFingerprintsJsonFilePath); file.is_open()) {
            nlohmann::json readModelFingerprints;
            file >> readModelFingerprints;
            modelFingerprints = readModelFingerprints.get<ModelFingerprintsMap>();
            file.close();
        }
        if (std::ifstream file(modelFingerprintsInstancesJsonFilePath); file.is_open()) {
            nlohmann::json readModelFingerprintsInstances;
            file >> readModelFingerprintsInstances;
            modelFingerprintsInstances = readModelFingerprintsInstances.get<ModelFingerprintsInstancesMap>();
            file.close();
        }
    }

    void StateManager::Export() {
        if (std::ofstream file(modelFingerprintsJsonFilePath); file.is_open()) {
            std::unordered_map<ModelFingerprintHash, nlohmann::json> modelFingerprintsToSave;
            for (const auto& [modelFingerprintHash, modelFingerprint] : modelFingerprints) {
                if (!modelFingerprint.name.empty() || modelFingerprint.tracked) {
                    const nlohmann::json modelFingerprintJson = modelFingerprint;
                    modelFingerprintsToSave[modelFingerprintHash] = modelFingerprintJson;
                }
            }
            file << modelFingerprintsToSave;
            file.close();
        }
        if (std::ofstream file(modelFingerprintsInstancesJsonFilePath); file.is_open()) {
            std::unordered_map<ModelFingerprintHash, std::unordered_map<ModelInstanceHash, nlohmann::json>>
                modelFingerprintsInstancesToSave;
            for (const auto& [modelFingerprintHash, modelFingerprintInstances] : modelFingerprintsInstances) {
                for (const auto& [modelInstanceHash, modelInstance] : modelFingerprintInstances) {
                    if (modelInstance.tracked) {
                        const nlohmann::json modelInstanceJson = modelInstance;
                        modelFingerprintsInstancesToSave[modelFingerprintHash][modelInstanceHash] = modelInstanceJson;
                    }
                }
            }
            file << modelFingerprintsInstancesToSave;
            file.close();
        }
    }

    StateManager stateManager;
}
