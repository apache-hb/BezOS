#pragma once

#include "apic.hpp"
#include "isr.hpp"

namespace hid {
    enum class Ps2ControllerStatus {
        eOk,
        eControllerSelfTestFailed,
        ePortTestFailed,
    };

    enum class Ps2DeviceType {
        eDisabled,

        eAtKeyboard,
        eMouse,
        eMouseWithScroll,
        eMouse5Button,

        eMf2Keyboard,
        eShortKeyboard,
        eN97Keyboard,
        e122KeyKeyboard,

        eJapaneseKeyboardG,
        eJapaneseKeyboardP,
        eJapaneseKeyboardA,

        eSunKeyboard,
    };

    constexpr bool isKeyboardDevice(Ps2DeviceType type) {
        switch (type) {
        case Ps2DeviceType::eAtKeyboard:
        case Ps2DeviceType::eMf2Keyboard:
        case Ps2DeviceType::eShortKeyboard:
        case Ps2DeviceType::eN97Keyboard:
        case Ps2DeviceType::e122KeyKeyboard:
        case Ps2DeviceType::eJapaneseKeyboardG:
        case Ps2DeviceType::eJapaneseKeyboardP:
        case Ps2DeviceType::eJapaneseKeyboardA:
        case Ps2DeviceType::eSunKeyboard:
            return true;
        default:
            return false;
        }
    }

    struct Ps2Device {
        Ps2DeviceType type;
        void(*write)(uint8_t);

        bool valid() const { return type != Ps2DeviceType::eDisabled; }

        bool isKeyboard() const { return isKeyboardDevice(type); }

        void enable();
        void disable();
    };

    class Ps2Controller {
        Ps2Device mKeyboard;
        Ps2Device mMouse;

    public:
        Ps2Controller(Ps2Device channel1, Ps2Device channel2)
            : mKeyboard(channel1)
            , mMouse(channel2)
        { }

        Ps2Controller()
            : mKeyboard(Ps2Device { Ps2DeviceType::eDisabled })
            , mMouse(Ps2Device { Ps2DeviceType::eDisabled })
        { }

        Ps2Device keyboard() const { return mKeyboard; }
        Ps2Device mouse() const { return mMouse; }

        bool hasKeyboard() const { return mKeyboard.valid(); }
        bool hasMouse() const { return mMouse.valid(); }

        uint8_t read() const;

        void enableIrqs(bool first, bool second);
    };

    struct Ps2ControllerResult {
        Ps2Controller controller;
        Ps2ControllerStatus status;

        operator bool() const { return status != Ps2ControllerStatus::eOk; }
    };

    Ps2ControllerResult EnablePs2Controller();

    void InstallPs2DeviceIsr(km::IoApicSet& ioApicSet, Ps2Device& device, const km::IApic *target, uint8_t isr);
}

template<>
struct km::Format<hid::Ps2ControllerStatus> {
    using String = stdx::StaticString<32>;
    static String toString(hid::Ps2ControllerStatus status);
};

template<>
struct km::Format<hid::Ps2DeviceType> {
    static void format(km::IOutStream& out, hid::Ps2DeviceType type);
};
