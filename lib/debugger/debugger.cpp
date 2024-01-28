//
// Created by Kissy on 1/6/2024.
//

#include "debugger.hpp"

#include "imgui.h"
#include "state_manager.hpp"
#include "imgui/backends/imgui_impl_dx11.h"

using json = nlohmann::json;

namespace debugger {
    void Debugger::Display() {
        modelFingerprintsWindow.Render();

        // TODO move to class?
        auto& modelFingerprints = interceptor::stateManager.GetModelFingerprints();
        for (auto& fingerprintsInstances : interceptor::stateManager.GetModelFingerprintsInstances() | std::views::values) {
            for (auto& fingerprintInstance : fingerprintsInstances | std::views::values) {
                if (!fingerprintInstance.tracked) {
                    continue;
                }
                
                auto& modelFingerprint = modelFingerprints[fingerprintInstance.modelFingerprintHash];
                if (!modelFingerprint.tracked) {
                    continue;
                }

                for (auto& modelDraw : fingerprintInstance.draws) {
                    ImVec2 topLeft(modelDraw.screenModelBoundingBox[0].x, modelDraw.screenModelBoundingBox[0].y);
                    ImVec2 bottomRight(modelDraw.screenModelBoundingBox[1].x, modelDraw.screenModelBoundingBox[1].y);
                    ImGui::GetForegroundDrawList()->AddText(topLeft, IM_COL32(0, 255, 255, 200), modelFingerprint.name.c_str());
                    ImGui::GetForegroundDrawList()->AddRect(topLeft, bottomRight, IM_COL32(0, 255, 255, 200));
                }
            }
        }

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    Debugger debugger;
}
