#pragma once

#include "uart.hpp"

namespace hid {
    enum class Ps2ControllerStatus {
        eOk,
        eControllerSelfTestFailed,
        ePortTestFailed,
    };

    struct Ps2Device {

    };

    class Ps2Controller {
        bool mChannel1;
        bool mChannel2;

    public:
        Ps2Controller(bool channel1, bool channel2)
            : mChannel1(channel1)
            , mChannel2(channel2)
        { }

        bool hasChannel1() const { return mChannel1; }
        bool hasChannel2() const { return mChannel2; }
    };

    struct Ps2ControllerResult {
        Ps2Controller controller;
        Ps2ControllerStatus status;

        operator bool() const { return status != Ps2ControllerStatus::eOk; }
    };

    Ps2ControllerResult EnablePs2Controller();
}

template<>
struct km::StaticFormat<hid::Ps2ControllerStatus> {
    using String = stdx::StaticString<32>;
    static String toString(hid::Ps2ControllerStatus status);
};
