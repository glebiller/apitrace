//
// Created by Kissy on 1/6/2024.
//

#include "debugger.hpp"

#include <d3d11_1.h>
#include <filesystem>
#include <sstream>
#include <string>

#include "models_fingerprints.hpp"
#include "state_manager.hpp"
#include "d3d11state.hpp"
#include "image.hpp"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "base64.hpp"
#include "dumper.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace interceptor {
    LRESULT Debugger::WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        return ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
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

        NewFrame();
    }

    void Debugger::NewFrame() {
        if (CaptureThisFrame()) {
            const std::string captuerFileDirectory = "E:/BlizzardLibrary/Heroes of the Storm/Versions/Base91418/Frame" + std::to_string(stateManager.CurrentFrame());
            create_directory(std::filesystem::path(captuerFileDirectory));

            const std::string captuerFileName = captuerFileDirectory + "/frame.html";
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
        //ImGui::ShowDemoWindow();

        ImGui::Begin("Debug");

        const bool captureNextFrameClicked = ImGui::Button("Capture next frame");
        if (!captureNextFrame && captureNextFrameClicked) {
            captureFrameNumber = stateManager.CurrentFrame() + 1;
        }
        captureNextFrame = captureNextFrameClicked;

        ImGui::End();

        if (CaptureThisFrame()) {
            frameCaptureFile << "</table></body></html>";
            frameCaptureFile.close();
        }

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Debugger::ReportDraw(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
        if (!CaptureThisFrame()) {
            return;
        }

        auto model = "unknown";
        if (stateManager.CurrentModel() == JAINA) {
            model = "jaina";
        } else if (stateManager.CurrentModel() == TEST) {
            model = "test";
        }

        frameCaptureFile << "<tr>";
        frameCaptureFile << "<td>" << stateManager.CurrentExecution().drawCall << "</td>";
        const std::string renderTargetImageBase64 = GetRenderTargetInBase64(stateManager.Context().pImmediateContext);
        frameCaptureFile << R"(<td><img width="512" src="data:image/jpeg;base64, )" << renderTargetImageBase64 << "\" /></td>";
        frameCaptureFile << "<td>Texture 0, RGB</td>";
        frameCaptureFile << "<td>Texture 0, Alpha</td>";
        frameCaptureFile << "<td>" << model << "</td>";
        frameCaptureFile << "<td>" << stateManager.CurrentModel().iaIndexBufferByteWidth << "</td>";
        frameCaptureFile << "<td>" << stateManager.CurrentModel().iaVertexBufferStride << "</td>";
        frameCaptureFile << "<td>" << stateManager.CurrentModel().iaVertexBufferByteWidth << "</td>";
        frameCaptureFile << "<td>(" << IndexCount << ", " << StartIndexLocation << ", " << BaseVertexLocation << ")</td>";
        frameCaptureFile << "<td>" << stateManager.CurrentModel().psConstantBufferByteWidth << "</td>";
        const auto position = stateManager.GetCurrentDrawPosition();
        frameCaptureFile << "<td>(" << position.x << ", " << position.y << ")</td>";
        frameCaptureFile << "</tr>\n";
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

    void InterceptID3D11DeviceContextUpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource,
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
