#pragma once

#include "uart.hpp"

namespace hid {
    enum class Ps2ControllerStatus {
        eOk,
        eSelfTestFailed,
    };

    class Ps2Controller {

    public:
        Ps2Controller() { }
    };

    struct OpenPs2ControllerResult {
        Ps2Controller controller;
        Ps2ControllerStatus status;

        operator bool() const { return status != Ps2ControllerStatus::eOk; }
    };

    OpenPs2ControllerResult OpenPs2Controller();
}
