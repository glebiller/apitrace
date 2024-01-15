//
// Created by Kissy on 1/6/2024.
//

#include "debugger.hpp"

#include <d3d11_1.h>
#include <filesystem>
#include <string>

#include "models_fingerprints.hpp"
#include "state_manager.hpp"
#include "json/json.hpp"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "dumper.hpp"
#include "helpers.hpp"

using json = nlohmann::json;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace interceptor {
    static std::string modelJsonFilePath = "E:/BlizzardLibrary/Heroes of the Storm/Versions/Base91418/models.json";

    bool Debugger::WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            return true;
        }
        if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) {
            return true;
        }

        return false;
    }

    void Debugger::Init(const HWND window, ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX11_Init(pDevice, pImmediateContext);

        std::ifstream modelNamesFile(modelJsonFilePath);
        if (modelNamesFile.is_open()) {
            modelNames = json::parse(modelNamesFile);
        }

        NewFrame();
    }

    void Debugger::NewFrame() {
        for (auto& modelDebug : modelDebugs) {
            modelDebug.instances.clear();
        }

        if (CaptureThisFrame()) {
            const std::string captuerFileName = "E:/BlizzardLibrary/Heroes of the Storm/Versions/Base91418/Frame" + std::to_string(stateManager.CurrentFrame()) + ".html";
            frameCaptureFile.open(captuerFileName, std::ios::out);
            frameCaptureFile << "<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"default.css\" /></head><body>\n";
            frameCaptureFile << "<h4 style=\"margin-left: 10px;\">Final Render Target:</h4><br>\n";
            const std::string captureFinalImageName = "Frame" + std::to_string(stateManager.CurrentFrame()) + ("Final.png");
            frameCaptureFile << "<img style=\"margin-left: 10px;\" src=\"" << captureFinalImageName << "\" /><br><br><h4 style=\"margin-left: 10px;\">All Render Events:</h4><br>\n";
            frameCaptureFile << "<table border=\"1\" style=\"text-align: center; margin-left: 8px;\"><tr>";
            frameCaptureFile << "<td>Index</td>";
            frameCaptureFile << "<td>Render Target</td>";
            frameCaptureFile << "<td>Texture 0, RGB</td>";
            frameCaptureFile << "<td>Texture 0, Alpha</td>";
            frameCaptureFile << "<td>Detected Object</td>";
            frameCaptureFile << "<td>IA Index ByteWidth</td>";
            frameCaptureFile << "<td>IA Vertex Stride</td>";
            frameCaptureFile << "<td>IA Vertex ByteWidth</td>";
            frameCaptureFile << "<td>(Index Count, Start Location, Base Vertex Location)</td>";
            frameCaptureFile << "<td>PS Constant ByteWidth</td>";
            frameCaptureFile << "<td>Coordinates</td>";
            frameCaptureFile << "</tr>\n";
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void Debugger::EndFrame() {
        /*ImGui::Begin("Debug");

        const bool captureNextFrameClicked = ImGui::Button("Capture next frame");
        if (!captureNextFrame && captureNextFrameClicked) {
            captureFrameNumber = stateManager.CurrentFrame() + 1;
        }
        captureNextFrame = captureNextFrameClicked;

        ImGui::End();*/

        if (ImGui::Begin("Models")) {
            if (ImGui::Button("Clear")) {
                modelDebugs.clear();
                createdInstanceCount = 0;
            }

            ImGui::SameLine();

            if (ImGui::Button("Export")) {
                if (std::ofstream file(modelJsonFilePath); file.is_open()) {
                    std::map<ModelFingerprint, std::string> mappedModelNames;
                    for (const auto& pair : modelNames) {
                        if (!pair.second.empty()) {
                            mappedModelNames.insert(pair);
                        }
                    }
                    const json modelNamesJson = mappedModelNames;
                    file << modelNamesJson;
                    file.close();
                }
            }

            ImGui::Separator();

            ImGui::Checkbox("With Instances", &displayModelsWithInstancesOnly);

            ImGui::Separator();

            static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingStretchSame;
            if (ImGui::BeginTable("Models", 9, flags)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("IA VB Stride", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("IA VB Bytes", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("IA IB Bytes", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("VS CB Bytes", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("PS CB Bytes", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
                ImGui::TableHeadersRow();

                // Filter
                bool displayedSelectedModel = false;
                std::vector<ModelDebug*> filteredModelDebugs;
                filteredModelDebugs.reserve(modelDebugs.size());
                for (auto& modelDebug : modelDebugs) {
                    if (displayModelsWithInstancesOnly && modelDebug.instances.empty()) {
                        continue;
                    }
                    if (&modelDebug == selectedModel) {
                        displayedSelectedModel = true;
                    }
                    filteredModelDebugs.push_back(&modelDebug);
                }
                if (!displayedSelectedModel) {
                    selectedModel = nullptr;
                    modelInstancesOpen = false;
                }

                ImGuiListClipper clipper;
                clipper.Begin(filteredModelDebugs.size());
                while (clipper.Step())
                    for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
                        ModelDebug* item = filteredModelDebugs[row_n];
                        ImGui::PushID(item->id);
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("%04d", item->id);
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                        auto foundModelName = modelNames.find(item->fingerprint);
                        if (foundModelName != modelNames.end()) {
                            ImGui::InputText("", &foundModelName->second, ImGuiInputTextFlags_None);
                        }
                        ImGui::PopItemWidth();
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", item->fingerprint.iaVertexBufferStride);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", item->fingerprint.iaVertexBufferByteWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", item->fingerprint.iaIndexBufferByteWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", item->fingerprint.vsConstantBufferByteWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", item->fingerprint.psConstantBufferByteWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(item->instances.size()));
                        ImGui::TableNextColumn();
                        if (item != selectedModel && ImGui::SmallButton("Details")) {
                            selectedModel = item;
                            modelInstancesOpen = true;
                        }
                        ImGui::PopID();
                    }
                ImGui::EndTable();
            }
        }
        ImGui::End();

        if (selectedModel != nullptr) {
            constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
            if (ImGui::Begin("Model Instances", &modelInstancesOpen, windowFlags)) {
                static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingStretchSame;
                if (ImGui::BeginTable("Models Instances", 10, flags)) {
                    ImGui::TableSetupColumn("Index Count", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Start Index Location", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Base Vertex Location", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("VP W", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                    ImGui::TableSetupColumn("VP H", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                    ImGui::TableSetupColumn("VP TL-X", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("VP TL-Y", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("x", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                    ImGui::TableSetupColumn("y", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
                    ImGui::TableHeadersRow();

                    for (auto instance : selectedModel->instances) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", instance.IndexCount);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", instance.StartIndexLocation);
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", instance.BaseVertexLocation);
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(instance.viewportWidth));
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(instance.viewportHeight));
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(instance.viewportTopLeftX));
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(instance.viewportTopLeftY));
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(instance.x));
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(instance.y));

                        ImGui::TableNextColumn();
                        if (selectedModelInstances.find(&instance) == selectedModelInstances.end() && ImGui::SmallButton("Track")) {
                            selectedModelInstances.insert(&instance);
                        }

                        //const auto center = ImVec2(instance.x, instance.y + 5);
                        //ImGui::GetBackgroundDrawList()->AddCircle(center, 10, IM_COL32(0, 255, 0, 200), 0, 4);
                    }
                    ImGui::EndTable();
                }
            }
            if (!modelInstancesOpen) {
                selectedModel = nullptr;
            }
            ImGui::End();
        }

        if (CaptureThisFrame()) {
            frameCaptureFile << "</table></body></html>";
            frameCaptureFile.close();
        }

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Debugger::ReportDraw(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
        const auto currentPosition = stateManager.GetCurrentDrawPosition();
        ModelFingerprint modelFingerprint{};
        memcpy(&modelFingerprint, &stateManager.CurrentModel(), sizeof(ModelFingerprint));
        const ModelInstance modelInstance{
            IndexCount, StartIndexLocation, BaseVertexLocation,
            stateManager.CurrentViewport().width, stateManager.CurrentViewport().height,
            stateManager.CurrentViewport().topLeftX, stateManager.CurrentViewport().topLeftY,
            currentPosition.x, currentPosition.y
        };
        const auto foundModelInstance = std::find_if(modelDebugs.begin(), modelDebugs.end(), [&](const ModelDebug &modelDebug) {
            return modelDebug.fingerprint == modelFingerprint;
        });
        if (foundModelInstance != modelDebugs.end()) {
            foundModelInstance->instances.push_back(modelInstance);
        } else {
            modelNames[modelFingerprint];
            const auto modelDebug = ModelDebug{createdInstanceCount++, modelFingerprint, {modelInstance}};
            modelDebugs.push_back(modelDebug);
        }

        /*if (stateManager.CurrentFrame() == 464 && stateManager.CurrentExecution().drawCall == 181) {
            std::vector<char> buffer;
            getCurrentVertexBuffer(stateManager.Context().pDevice, stateManager.Context().pImmediateContext,
                stateManager.CurrentModel().iaVertexBufferStride, &buffer);
            writeBufferToFile("C:/Users/guill/Workspace/apitrace/log/vertex-buffer.bin", buffer);
            std::vector<char> buffer2;
            getCurrentVertexBuffer(stateManager.Context().pDevice, stateManager.Context().pImmediateContext,
                stateManager.CurrentModel().iaVertexBufferStride, &buffer2);
            writeBufferToFile("C:/Users/guill/Workspace/apitrace/log/index-buffer.bin", buffer2);
        }*/

        if (!CaptureThisFrame()) {
            return;
        }

        auto model = "unknown";
        if (stateManager.CurrentModel() == WHITEMANE) {
            model = "whitemane";
        } else if (stateManager.CurrentModel() == TEST) {
            model = "test";
        }

        frameCaptureFile << "<tr>";
        frameCaptureFile << "<td>" << stateManager.CurrentExecution().drawCall << "</td>";
        const std::string renderTargetImageBase64 = GetRenderTargetInBase64(stateManager.Context().pDevice, stateManager.Context().pImmediateContext);
        frameCaptureFile << R"(<td><img width="512" src="data:image/jpeg;base64, )" << renderTargetImageBase64 << "\" /></td>";
        frameCaptureFile << "<td>Texture 0, RGB</td>";
        frameCaptureFile << "<td>Texture 0, Alpha</td>";
        frameCaptureFile << "<td>" << model << "</td>";
        frameCaptureFile << "<td>" << stateManager.CurrentModel().iaIndexBufferByteWidth << "</td>";
        frameCaptureFile << "<td>" << stateManager.CurrentModel().iaVertexBufferStride << "</td>";
        frameCaptureFile << "<td>" << stateManager.CurrentModel().iaVertexBufferByteWidth << "</td>";
        frameCaptureFile << "<td>(" << IndexCount << ", " << StartIndexLocation << ", " << BaseVertexLocation << ")</td>";
        frameCaptureFile << "<td>" << stateManager.CurrentModel().psConstantBufferByteWidth << "</td>";
        frameCaptureFile << "<td>(" << currentPosition.x << ", " << currentPosition.y << ")</td>";
        frameCaptureFile << "</tr>\n";
    }

    void Debugger::Shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    /*
    void InterceptID3D11DeviceCreateVertexShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
                                                 ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader) {

        if (BytecodeLength == 3260) {
            const std::string_view shaderBytecodeView(static_cast<const char*>(pShaderBytecode), BytecodeLength);
            const size_t hash = std::hash<std::string_view>{}(shaderBytecodeView);
            std::ostringstream shaderFileName;
            shaderFileName << "C:/Users/guill/Workspace/apitrace/log/vs_" << hash << ".txt";
            std::ofstream(shaderFileName.str(), std::ios::binary).write(static_cast<const char*>(pShaderBytecode), BytecodeLength);
        }
    }

    void InterceptID3D11DeviceContext
Subresource(ID3D11Resource* pDstResource, UINT DstSubresource,
                                                       const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) {

        if (SrcRowPitch == 267408 && SrcDepthPitch == 267408) {
            const std::string_view vertexBufferView(static_cast<const char*>(pSrcData), pDstBox->right);
            const size_t hash = std::hash<std::string_view>{}(vertexBufferView);
            std::ostringstream shaderFileName;
            shaderFileName << "C:/Users/guill/Workspace/apitrace/log/vb_" << hash << ".txt";
            std::ofstream(shaderFileName.str(), std::ios::binary).write(static_cast<const char*>(pSrcData), pDstBox->right);
        }
    }
        */

    bool Debugger::CaptureThisFrame() const {
        return captureFrameNumber == stateManager.CurrentFrame();
    }

    Debugger debugger;
}
