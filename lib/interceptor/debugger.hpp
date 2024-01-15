//
// Created by Kissy on 1/6/2024.
//

#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <windows.h>

#include "models_fingerprints.hpp"

namespace interceptor {

    struct ModelInstance {
        UINT IndexCount;
        UINT StartIndexLocation;
        INT BaseVertexLocation;
        float viewportWidth;
        float viewportHeight;
        float viewportTopLeftX;
        float viewportTopLeftY;
        float x;
        float y;

        [[nodiscard]] std::string toString() const {
            std::ostringstream name;
            name << IndexCount << "," << StartIndexLocation << "," << BaseVertexLocation
                << "," << viewportWidth << "," << viewportHeight << "," << viewportTopLeftX << "," << viewportTopLeftY
                << " @ " << x << "," << y;
            return name.str();
        }

        bool operator==(const ModelInstance& rhs) const {
            return IndexCount == rhs.IndexCount
                && StartIndexLocation == rhs.StartIndexLocation
                && BaseVertexLocation == rhs.BaseVertexLocation
                && viewportWidth == rhs.viewportWidth
                && viewportHeight == rhs.viewportHeight
                && viewportTopLeftX == rhs.viewportTopLeftX
                && viewportTopLeftY == rhs.viewportTopLeftY;
        }
        bool operator<(const ModelInstance& other) const {
            return std::tie(IndexCount, StartIndexLocation, BaseVertexLocation, viewportWidth, viewportHeight, viewportTopLeftX, viewportTopLeftY) <
                   std::tie(other.IndexCount, other.StartIndexLocation, other.BaseVertexLocation, other.viewportWidth, other.viewportHeight, other.viewportTopLeftX, other.viewportTopLeftY);
        }
    };

    struct ModelDebug {
        long id;
        ModelFingerprint fingerprint;
        std::vector<ModelInstance> instances;

        bool operator==(const ModelDebug& other) const {
            return id == other.id;
        }
        bool operator<(const ModelDebug& other) const {
            return fingerprint < other.fingerprint;
        }
    };

    class Debugger {
        bool captureNextFrame = false;
        int captureFrameNumber = -1;
        //int captureFrameNumber = 464;
        std::ofstream frameCaptureFile;
        ModelDebug* selectedModel = nullptr;
        std::set<ModelInstance*> selectedModelInstances;
        int createdInstanceCount = 0;
        std::vector<ModelDebug> modelDebugs;
        std::map<ModelFingerprint, std::string> modelNames;
        bool modelInstancesOpen = false;
        bool displayModelsWithInstancesOnly = false;
    public:
        static bool WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void Init(HWND window, ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext);
        void NewFrame();

        void EndFrame();

        void ReportDraw(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

        void Shutdown();

    private:
        bool CaptureThisFrame() const;
    };

    /**
    * Singleton.
    */
    extern Debugger debugger;
}
