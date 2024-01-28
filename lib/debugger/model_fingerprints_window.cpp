#include "model_fingerprints_window.hpp"

#include <fstream>

#include "imgui/imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "state_manager.hpp"

namespace debugger {

    void ModelFingerprintsWindow::Render() {
        const ImGuiStyle& style = ImGui::GetStyle();
        auto& modelFingerprints = interceptor::stateManager.GetModelFingerprints();
        auto& modelFingerprintsInstances = interceptor::stateManager.GetModelFingerprintsInstances();

        if (ImGui::Begin("Model Fingerprints")) {
            if (ImGui::Button("Clear")) {
                // TODO update
                interceptor::stateManager.ClearModelFingerprints();
            }

            ImGui::SameLine();

            if (ImGui::Button("Import")) {
                interceptor::stateManager.Import();
            }

            ImGui::SameLine();

            if (ImGui::Button("Export")) {
                interceptor::stateManager.Export();
            }

            ImGui::SameLine();
            ImGui::Checkbox("Named", &displayOnlyNamed);

            ImGui::SameLine();
            ImGui::Checkbox("Drawn", &displayOnlyDrawn);

            ImGui::SameLine();
            ImGui::Checkbox("Tracked", &displayOnlyTracked);

            static ImGuiTableFlags flags = ImGuiTableFlags_Sortable 
                | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX
                | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders
                | ImGuiTableFlags_SizingFixedFit;
            if (ImGui::BeginTable("Models", 12, flags)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_None, 4 * 7.0f);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 15 * 7.0f);
                ImGui::TableSetupColumn("Index\nBytes", ImGuiTableColumnFlags_None, 7 * 7.0f);
                ImGui::TableSetupColumn("Vertex\nCount", ImGuiTableColumnFlags_None, 8 * 7.0f);
                ImGui::TableSetupColumn("Vertex\nStride", ImGuiTableColumnFlags_None, 8 * 7.0f);
                ImGui::TableSetupColumn("Vertex\nBytes", ImGuiTableColumnFlags_None,8 * 7.0f);
                ImGui::TableSetupColumn("VC\nCount", ImGuiTableColumnFlags_None, 7 * 7.0f);
                ImGui::TableSetupColumn("VC\nBytes", ImGuiTableColumnFlags_None, 7 * 7.0f);
                ImGui::TableSetupColumn("PC\nCount", ImGuiTableColumnFlags_None, 7 * 7.0f);
                ImGui::TableSetupColumn("PC\nBytes", ImGuiTableColumnFlags_None, 7 * 7.0f);
                ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_None, 7 * 7.0f);
                ImGui::TableSetupColumn("Track", ImGuiTableColumnFlags_NoSort, 5 * 7.0f);
                ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
                ImGui::TableHeadersRow();

                // Filter // TODO avoid sorting every frame?

                displayedModelFingerprints.clear();
                displayedModelFingerprints.reserve(modelFingerprints.size());
                for (auto& modelFingerprint : modelFingerprints | std::views::values) {
                    if (displayOnlyNamed && modelFingerprint.name.empty()) {
                        continue;
                    }
                    if (displayOnlyDrawn && modelFingerprint.drawCount == 0) {
                        continue;
                    }
                    if (displayOnlyTracked && !modelFingerprint.tracked) {
                        continue;
                    }
                    displayedModelFingerprints.push_back(&modelFingerprint);
                }

                ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
                auto sortComparator = [](const interceptor::ModelFingerprint* lhs, const interceptor::ModelFingerprint* rhs) {
                    // TODO use sortSpecs instead
                    return lhs->id < rhs->id;
                };
                std::ranges::sort(displayedModelFingerprints, sortComparator);

                ImGuiListClipper clipper;
                clipper.Begin(displayedModelFingerprints.size());
                while (clipper.Step()) {
                    for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
                        const auto& modelFingerprint = displayedModelFingerprints[row_n];

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        char label[32];
                        sprintf(label, "%04d", modelFingerprint->id);
                        const bool isSelected = selectedModelFingerprintHash == modelFingerprint->hash;
                        if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                            if (selectedModelFingerprintHash == modelFingerprint->hash) {
                                selectedModelFingerprintHash = 0;
                                modelInstancesWindow.Close();
                            } else {
                                selectedModelFingerprintHash = modelFingerprint->hash;
                                modelInstancesWindow.Open();
                            }
                        }
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 0));
                        ImGui::PushID((std::string("name-") + std::to_string(modelFingerprint->hash)).c_str());
                        ImGui::InputText("", &modelFingerprint->name, ImGuiInputTextFlags_None);
                        ImGui::PopID();
                        ImGui::PopStyleVar();
                        ImGui::PopItemWidth();
                        ImGui::TableNextColumn();
                        ImGui::Text("%7u", modelFingerprint->iaIndexBufferByteWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%8u", modelFingerprint->iaVertexBuffersCount);
                        ImGui::TableNextColumn();
                        ImGui::Text("%8u", modelFingerprint->iaVertexBufferStrides);
                        ImGui::TableNextColumn();
                        ImGui::Text("%8u", modelFingerprint->iaVertexBufferByteWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%7u", modelFingerprint->vsConstantBuffersCount);
                        ImGui::TableNextColumn();
                        ImGui::Text("%7u", modelFingerprint->vsConstantBufferByteWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%7u", modelFingerprint->psConstantBuffersCount);
                        ImGui::TableNextColumn();
                        ImGui::Text("%7u", modelFingerprint->psConstantBufferByteWidth);
                        ImGui::TableNextColumn();
                        ImGui::Text("%7u", modelFingerprint->drawCount);
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 0));
                        ImGui::TableNextColumn();
                        ImGui::PushID((std::string("tracked-") + std::to_string(modelFingerprint->hash)).c_str());
                        ImGui::Checkbox("", &modelFingerprint->tracked);
                        ImGui::PopID();
                        ImGui::PopStyleVar();
                    }
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        modelInstancesWindow.Render(this->selectedModelFingerprintHash);
    }
}
