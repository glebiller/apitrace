#include "model_instance_details_window.hpp"

#include "debugger.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include "state_manager.hpp"


struct ImGuiStyle;

namespace debugger {
    void ModelInstanceDetailsWindow::Open() {
        open = true;
    }

    void ModelInstanceDetailsWindow::Render(interceptor::ModelFingerprintHash modelFingerprintHash,
                                            interceptor::ModelInstanceHash modelInstanceHash) {
        if (!open) {
            return;
        }
        if (modelFingerprintHash == 0 || modelInstanceHash == 0) {
            Close();
            return;
        }

        const auto& modelFingerprints = interceptor::stateManager.GetModelFingerprints();
        const auto& foundModelFingerprint = modelFingerprints.find(modelFingerprintHash);
        if (foundModelFingerprint == modelFingerprints.end()) {
            Close();
            return;
        }

        auto& modelFingerprintsInstances = interceptor::stateManager.GetModelFingerprintsInstances();
        const auto& foundModelFingerprintInstances = modelFingerprintsInstances.find(modelFingerprintHash);
        if (foundModelFingerprintInstances == modelFingerprintsInstances.end()) {
            Close();
            return;
        }

        const auto& modelFingerprint = foundModelFingerprint->second;
        const auto& foundModelFingerprintInstance = foundModelFingerprintInstances->second.find(modelInstanceHash);
        if (foundModelFingerprintInstance == foundModelFingerprintInstances->second.end()) {
            Close();
            return;
        }

        auto& modelInstance = foundModelFingerprintInstance->second;

        if (ImGui::Begin("Model Instance Details", &open, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
            const ImU32 prefixCellBg = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 0.65f));
            constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX;


            if (ImGui::CollapsingHeader("Bounding Box")) {
                ImGui::SliderFloat3("Center Offset", modelInstance.modelCenterOffset.m128_f32, -3, 3);
                ImGui::SliderFloat3("Box Scale", modelInstance.boundingBoxScale.m128_f32, -1, 1);

                if (ImGui::BeginTable("Model Bounding Box", 4, tableFlags)) {
                    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 14 * 7.0f);
                    ImGui::TableSetupColumn("x", ImGuiTableColumnFlags_WidthFixed, 8 * 7.0f);
                    ImGui::TableSetupColumn("y", ImGuiTableColumnFlags_WidthFixed, 8 * 7.0f);
                    ImGui::TableSetupColumn("z", ImGuiTableColumnFlags_WidthFixed, 8 * 7.0f);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, prefixCellBg);
                    ImGui::TextUnformatted("Model Center");
                    ImGui::TableNextColumn();
                    ImGui::Text("%8.3f", modelInstance.modelCenter.x);
                    ImGui::TableNextColumn();
                    ImGui::Text("%8.3f", modelInstance.modelCenter.y);
                    ImGui::TableNextColumn();
                    ImGui::Text("%8.3f", modelInstance.modelCenter.z);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, prefixCellBg);
                    ImGui::TextUnformatted("Box Size");
                    for (int i = 0; i < 3; i++) {
                        ImGui::TableNextColumn();
                        ImGui::Text("%8.3f", modelInstance.boundingBoxSize.m128_f32[i]);
                    }
                    ImGui::EndTable();
                }
            }

            if (ImGui::CollapsingHeader("Vertex Buffer")) {
                if (!modelInstance.tracked) {
                    ImGui::Text("Selected model is not tracked.");
                } else if (modelInstance.draws.empty()) {
                    ImGui::Text("Selected model is not drawn.");
                } else {
                    if (ImGui::BeginTabBar("Vertex Buffer", ImGuiTabBarFlags_None)) {
                        const auto& inputLayouts = interceptor::stateManager.GetInputLayouts();
                        const auto& foundInputLayout = inputLayouts.find(modelFingerprint.iaInputLayout);
                        if (foundInputLayout != inputLayouts.end()) {
                            // TODO handle multiple vertex buffers?
                            const auto& inputLayout = foundInputLayout->second;
                            for (const auto& input : inputLayout) {
                                if (ImGui::BeginTabItem(input.SemanticName)) {

                                    if (ImGui::BeginTable(input.SemanticName, 4, tableFlags)) {
                                        ImGui::TableSetupColumn("i", ImGuiTableColumnFlags_WidthFixed, 21.0f);
                                        ImGui::TableSetupColumn("x", ImGuiTableColumnFlags_WidthFixed, 6 * 7.0f);
                                        ImGui::TableSetupColumn("y", ImGuiTableColumnFlags_WidthFixed, 6 * 7.0f);
                                        ImGui::TableSetupColumn("z", ImGuiTableColumnFlags_WidthFixed, 6 * 7.0f);
                                        
                                        int row = 0;
                                        modelInstance.IterateVertices([prefixCellBg, &row](const float* vertex) {
                                            ImGui::TableNextRow();
                                            ImGui::TableNextColumn();
                                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, prefixCellBg);
                                            ImGui::Text("%03d", row);
                                            // TODO based on input layout
                                            for (int column = 0; column < 3; column++) {
                                                ImGui::TableNextColumn();
                                                ImGui::Text("%6.3f", vertex[column]);
                                            }
                                            row++;
                                            });

                                        ImGui::EndTable();
                                    }
                                    ImGui::EndTabItem();
                                }
                            }
                        }

                        ImGui::EndTabBar();
                    }
                }
            }

            // TODO each draw
            if (ImGui::CollapsingHeader("Constant Buffers")) {
                if (!modelInstance.tracked) {
                    ImGui::Text("Selected model is not tracked.");
                } else if (modelInstance.draws.empty()) {
                    ImGui::Text("Selected model is not drawn.");
                } else {
                    if (ImGui::BeginTabBar("Constant Buffers", ImGuiTabBarFlags_None)) {
                        if (ImGui::BeginTabItem("Transforms")) {
                            constexpr char stepOne = 1;
                            ImGui::InputScalar("World Transform Index", ImGuiDataType_U8, &modelInstance.viewTransformIndex,
                                &stepOne, nullptr, "%u");

                            ImGui::Text("World Transform");
                            // TODO factorise with one method displayMatrix?
                            if (ImGui::BeginTable("View Transform", 5, tableFlags)) {
                                ImGui::TableSetupColumn("i", ImGuiTableColumnFlags_WidthFixed, 21.0f);
                                ImGui::TableSetupColumn("x", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                                ImGui::TableSetupColumn("y", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                                ImGui::TableSetupColumn("z", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                                ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);

                                for (int row = 0; row < 4; row++) {
                                    ImGui::TableNextRow();
                                    ImGui::TableNextColumn();
                                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, prefixCellBg);
                                    ImGui::Text("%03d", row);
                                    for (int column = 0; column < 4; column++) {
                                        ImGui::TableNextColumn();
                                        // TODO handle multiple
                                        ImGui::Text("%10.7f", modelInstance.draws[0].worldTransform.r[row].m128_f32[column]);
                                    }
                                }
                                ImGui::EndTable();
                            }

                            ImGui::Text("View Transform");
                            if (ImGui::BeginTable("View Transform", 5, tableFlags)) {
                                ImGui::TableSetupColumn("i", ImGuiTableColumnFlags_WidthFixed, 21.0f);
                                ImGui::TableSetupColumn("x", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                                ImGui::TableSetupColumn("y", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                                ImGui::TableSetupColumn("z", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                                ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);

                                for (int row = 0; row < 4; row++) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, prefixCellBg);
                                    ImGui::Text("%03d", 197 + row);
                                    for (int column = 0; column < 4; column++) {
                                        ImGui::TableSetColumnIndex(column + 1);
                                        // TODO handle multiple
                                        ImGui::Text(
                                            "%10.7f", modelInstance.draws[0].viewTransform.r[row].m128_f32[column]);
                                    }
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }

                        // TODO handle draws Index
                        //RenderBufferTabItem("IB", modelInstance.draws[0].indexBuffers);
                        //RenderBufferTabItem("VB", modelInstance.draws[0].vertexBuffers);
                        RenderBufferTabItem("VCB", modelInstance.draws[0].vertexConstantBuffers);
                        RenderBufferTabItem("PCB", modelInstance.draws[0].pixelConstantBuffers);

                        ImGui::EndTabBar();
                    }
                }
            }

            ImGui::End();
        }

        if (modelInstance.tracked) {
            for (auto& modelDraw : modelInstance.draws) {
                // TODO create a method
                const auto modelCenter = ImVec2(modelDraw.projectedModelCenter.x, modelDraw.projectedModelCenter.y);
                ImGui::GetForegroundDrawList()->AddCircleFilled(modelCenter, 5, IM_COL32(255, 155, 0, 200));

                constexpr ImU32 color1 = IM_COL32(255, 0, 0, 100);
                constexpr ImU32 color2 = IM_COL32(255, 155, 0, 100);
                constexpr ImU32 color3 = IM_COL32(255, 255, 155, 100);
                for (int i = 0; i < 4; ++i) {
                    constexpr float thickness = 2.0f;
                    constexpr int nodePair[] = {1, 3, 0, 2};
                    ImVec2 p1(modelDraw.projectedBoundingBoxCorners[i].x,
                              modelDraw.projectedBoundingBoxCorners[i].y);
                    ImVec2 p2(modelDraw.projectedBoundingBoxCorners[nodePair[i]].x,
                              modelDraw.projectedBoundingBoxCorners[nodePair[i]].y);
                    // Draw lines for the bottom face
                    ImGui::GetForegroundDrawList()->AddLine(p1, p2, color1, thickness);
                    ImVec2 p3(modelDraw.projectedBoundingBoxCorners[i + 4].x,
                              modelDraw.projectedBoundingBoxCorners[i + 4].y);
                    ImVec2 p4(modelDraw.projectedBoundingBoxCorners[nodePair[i] + 4].x,
                              modelDraw.projectedBoundingBoxCorners[nodePair[i] + 4].y);
                    // Draw lines for the top face
                    ImGui::GetForegroundDrawList()->AddLine(p3, p4, color2, thickness);
                    // Draw lines connecting the bottom and top faces
                    ImGui::GetForegroundDrawList()->AddLine(p1, p3, color3, thickness);
                }

                for (int i = 0; i < 6; i++) {
                    ImVec2 center(modelDraw.projectedFacesCenters[i].x,
                        modelDraw.projectedFacesCenters[i].y);
                    ImGui::GetForegroundDrawList()->AddCircleFilled(center, 3, IM_COL32(155, 0, 255, 200));
                }
            }
        }
    }

    void ModelInstanceDetailsWindow::Close() {
        this->open = false;
    }

    void ModelInstanceDetailsWindow::RenderBufferTabItem(const char* const bufferName, const std::vector<std::vector<BYTE>>& buffers) {
        const auto prefixCellBg = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 0.65f));
        for (int bufferIndex = 0; bufferIndex < buffers.size(); bufferIndex++) {
            char label[32];
            sprintf(label, "%s %02d", bufferName, bufferIndex);
            if (ImGui::BeginTabItem(label)) {
                constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                    ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX;
                if (ImGui::BeginTable(label, 5, tableFlags)) {
                    ImGui::TableSetupColumn("i", ImGuiTableColumnFlags_WidthFixed, 21.0f);
                    ImGui::TableSetupColumn("x", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                    ImGui::TableSetupColumn("y", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                    ImGui::TableSetupColumn("z", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);
                    ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthFixed, 10 * 7.0f);

                    // TODO handle multiple
                    auto& buffer = buffers[bufferIndex];
                    for (int row = 0; row < buffer.size() / (4 * sizeof(float)); row++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, prefixCellBg);
                        ImGui::Text("%03d", row);
                        for (int column = 0; column < 4; column++) {
                            ImGui::TableSetColumnIndex(column + 1);
                            const float value = *reinterpret_cast<const float*>(&buffer[(row * 4 + column) * sizeof(float)]);
                            ImGui::Text("%10.7f", value);
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
        }
    }
}
