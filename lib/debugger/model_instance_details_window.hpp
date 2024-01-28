#pragma once

#include <DirectXMath.h>

#include "model_instance.hpp"

namespace debugger {

    class ModelInstanceDetailsWindow {
        bool open = false;

    public:
        void Open();
        void Render(interceptor::ModelFingerprintHash modelFingerprintHash, interceptor::ModelInstanceHash modelInstance);
        void Close();

    private:
        static void RenderBufferTabItem(const char* bufferName, const std::vector<std::vector<BYTE>>& buffers);
    };

}
