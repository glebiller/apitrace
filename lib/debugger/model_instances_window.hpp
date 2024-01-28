#pragma once
#include <string>
#include <unordered_map>

#include "model_fingerprint.hpp"
#include "model_instance.hpp"
#include "model_instance_details_window.hpp"


namespace debugger {

    class ModelInstancesWindow {
        bool displayOnlyDrawn = true;
        bool displayOnlyTracked = false;
        std::vector<interceptor::ModelInstance*> displayedModelInstances;
        interceptor::ModelInstanceHash selectedModelInstanceHash = 0;
        bool open = false;

        ModelInstanceDetailsWindow modelInstanceDetailsWindow;

    public:
        void Open();
        void Render(interceptor::ModelFingerprintHash modelFingerprintHash);
        void Close();
    };

}
