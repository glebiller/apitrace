//
// Created by Kissy on 1/6/2024.
//

#pragma once

#include "model_fingerprints_window.hpp"

namespace debugger {

    class Debugger {
        ModelFingerprintsWindow modelFingerprintsWindow;

    public:
        void Display();

    };

    /**
    * Singleton.
    */
    extern Debugger debugger;
}
