#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "model_fingerprint.hpp"
#include "model_instance.hpp"
#include "model_instances_window.hpp"


namespace debugger {

    using ModelFingerprintInstancesPair = std::pair<const interceptor::ModelFingerprint, std::vector<interceptor::ModelInstance>>;

    class ModelFingerprintsWindow {
        bool displayOnlyNamed = false;
        bool displayOnlyDrawn = false;
        bool displayOnlyTracked = false;
        std::vector<interceptor::ModelFingerprint*> displayedModelFingerprints;
        interceptor::ModelFingerprintHash selectedModelFingerprintHash = 0;

        ModelInstancesWindow modelInstancesWindow;

        // TODO remove / move into ModelFingerprint?
        std::unordered_map<std::size_t, std::string> modelNames;
        std::unordered_map<std::size_t, std::array<std::array<float, 4>, 3>> modelBoundingBoxes;

    public:
        void Render();

    };

}
