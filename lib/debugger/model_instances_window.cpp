#include "model_instances_window.hpp"

#include "imgui/imgui.h"
#include "state_manager.hpp"


namespace debugger {
    void ModelInstancesWindow::Open() {
        open = true;
    }

    void ModelInstancesWindow::Render(const interceptor::ModelFingerprintHash modelFingerprintHash) {
        if (!open || modelFingerprintHash == 0) {
            return;
        }

        auto& modelFingerprints = interceptor::stateManager.GetModelFingerprints();

        const auto foundModelFingerprint = modelFingerprints.find(modelFingerprintHash);
        if (foundModelFingerprint == modelFingerprints.end()) {
            Close();
            return;
        }

        const auto& modelFingerprint = foundModelFingerprint->second;
        auto& modelFingerprintsInstances = interceptor::stateManager.GetModelFingerprintsInstances();
        auto& modelFingerprintInstances = modelFingerprintsInstances[modelFingerprintHash];

        const ImGuiStyle& style = ImGui::GetStyle();
        if (ImGui::Begin("Model Instances", &open)) {
            if (ImGui::Button("Clear")) {
                interceptor::stateManager.ClearModelFingerprintsInstances();
            }

            if (modelFingerprint.tracked) {
                ImGui::SameLine();
                if (ImGui::Button("Untrack All")) {
                    for (auto& modelInstance : modelFingerprintInstances | std::views::values) {
                        modelInstance.tracked = false;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Track All")) {
                    for (auto& modelInstance : modelFingerprintInstances | std::views::values) {
                        modelInstance.tracked = true;
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Hide All")) {
                for (auto& modelInstance : modelFingerprintInstances | std::views::values) {
                    modelInstance.hidden = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Show All")) {
                for (auto& modelInstance : modelFingerprintInstances | std::views::values) {
                    modelInstance.hidden = false;
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("Drawn", &displayOnlyDrawn);

            ImGui::SameLine();
            ImGui::Checkbox("Tracked", &displayOnlyTracked);

            static ImGuiTableFlags flags = ImGuiTableFlags_Sortable
                | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX
                | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders
                | ImGuiTableFlags_SizingFixedFit;
            if (ImGui::BeginTable("Models Instances", 14, flags)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_None, 30.0f);
                ImGui::TableSetupColumn("Draw\nCalls", ImGuiTableColumnFlags_None, 7 * 7.0f);
                ImGui::TableSetupColumn("Index\nCount", ImGuiTableColumnFlags_None, 50.0f);
                ImGui::TableSetupColumn("Index\nStart", ImGuiTableColumnFlags_None, 50.0f);
                ImGui::TableSetupColumn("Vertex\nBase", ImGuiTableColumnFlags_None, 50.0f);
                ImGui::TableSetupColumn("Vertex\nOffset", ImGuiTableColumnFlags_None, 50.0f);
                ImGui::TableSetupColumn("PS\nCount", ImGuiTableColumnFlags_None, 50.0f);
                ImGui::TableSetupColumn("PS\nWidth", ImGuiTableColumnFlags_None, 50.0f);
                ImGui::TableSetupColumn("VP\nW", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("VP\nH", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("TL\nX", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("TL\nY", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("Track", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Hide", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
                ImGui::TableHeadersRow();

                displayedModelInstances.clear();
                displayedModelInstances.reserve(modelFingerprints.size());
                for (auto& modelInstance : modelFingerprintInstances | std::views::values) {
                    if (displayOnlyDrawn && modelInstance.draws.empty()) {
                        continue;
                    }
                    if (displayOnlyTracked && !modelInstance.tracked) {
                        continue;
                    }
                    displayedModelInstances.push_back(&modelInstance);
                }

                ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
                auto sortComparator = [](const interceptor::ModelInstance* lhs, const interceptor::ModelInstance* rhs) {
                    // TODO use sortSpecs instead
                    return lhs->id < rhs->id;
                    };
                std::ranges::sort(displayedModelInstances, sortComparator);

                ImGuiListClipper clipper;
                clipper.Begin(displayedModelInstances.size());
                while (clipper.Step()) {
                    for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
                        const auto& modelInstance = displayedModelInstances[row_n];

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        char label[32];
                        sprintf(label, "%04d", modelInstance->id);
                        const bool isSelected = selectedModelInstanceHash == modelInstance->hash;
                        if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                            if (selectedModelInstanceHash == modelInstance->hash) {
                                selectedModelInstanceHash = 0;
                                modelInstanceDetailsWindow.Close();
                            } else {
                                selectedModelInstanceHash = modelInstance->hash;
                                modelInstanceDetailsWindow.Open();
                            }
                        }
                        ImGui::TableNextColumn();
                        ImGui::Text("%7d", modelInstance->draws.size());
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", modelInstance->IndexCount);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", modelInstance->StartIndexLocation);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", modelInstance->BaseVertexLocation);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", modelInstance->IAVertexBufferOffsets);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", modelInstance->psShaderResourceNumElements);
                        ImGui::TableNextColumn();
                        ImGui::Text("%u", modelInstance->psShaderResourceWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%4d", static_cast<int>(modelInstance->viewport.width));
                        ImGui::TableNextColumn();
                        ImGui::Text("%4d", static_cast<int>(modelInstance->viewport.height));
                        ImGui::TableNextColumn();
                        ImGui::Text("%4d", static_cast<int>(modelInstance->viewport.topLeftX));
                        ImGui::TableNextColumn();
                        ImGui::Text("%4d", static_cast<int>(modelInstance->viewport.topLeftY));
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 0));
                        ImGui::TableNextColumn();
                        if (modelFingerprint.tracked) {
                            std::string trackedId = "tracked-" + std::to_string(modelInstance->hash);
                            ImGui::PushID(trackedId.c_str());
                            ImGui::Checkbox("", &modelInstance->tracked);
                            ImGui::PopID();
                        } else {
                            ImGui::Text("-");
                        }
                        ImGui::TableNextColumn();
                        std::string hiddenId = "hidden-" + std::to_string(modelInstance->hash);
                        ImGui::PushID(hiddenId.c_str());
                        ImGui::Checkbox("", &modelInstance->hidden);
                        ImGui::PopID();
                        ImGui::PopStyleVar();
                    }
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        modelInstanceDetailsWindow.Render(modelFingerprintHash, this->selectedModelInstanceHash);
    }

    void ModelInstancesWindow::Close() {
        this->open = false;
        modelInstanceDetailsWindow.Close();
    }
}
