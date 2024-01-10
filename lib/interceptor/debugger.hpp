//
// Created by Kissy on 1/6/2024.
//

#pragma once

#include <d3d11.h>
#include <fstream>
#include <windows.h>

#include "image.hpp"

namespace interceptor {
    class Debugger {
        bool captureNextFrame = false;
        //int captureFrameNumber = -1;
        int captureFrameNumber = 464;
        std::ofstream frameCaptureFile;
    public:
        static LRESULT WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void Init(HWND window, ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext);
        void NewFrame();

        void EndFrame();

        void ReportDraw(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

    private:
        bool CaptureThisFrame() const;
    };

    /**
    * Singleton.
    */
    extern Debugger debugger;
}
