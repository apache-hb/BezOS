#include "hid/hid.hpp"

#include "log.hpp"

#include "common/util/defer.hpp"

static hid::Ps2Controller gController;
static km::NotificationStream *gStream;
static km::Topic *gHidPs2Topic;

static constexpr OsKey kScanMap[UINT8_MAX] = {
    [0x1C] = eKeyA,
    [0x1B] = eKeyS,
    [0x23] = eKeyD,
    [0x2B] = eKeyF,
    [0x34] = eKeyG,
    [0x33] = eKeyH,
    [0x3B] = eKeyJ,
    [0x42] = eKeyK,
    [0x4B] = eKeyL,

    [0x15] = eKeyQ,
    [0x1D] = eKeyW,
    [0x24] = eKeyE,
    [0x2D] = eKeyR,
    [0x2C] = eKeyT,
    [0x35] = eKeyY,
    [0x3C] = eKeyU,
    [0x43] = eKeyI,
    [0x44] = eKeyO,
    [0x4D] = eKeyP,

    [0x1A] = eKeyZ,
    [0x22] = eKeyX,
    [0x21] = eKeyC,
    [0x2A] = eKeyV,
    [0x32] = eKeyB,
    [0x31] = eKeyN,
    [0x3A] = eKeyM,

    [0x29] = eKeySpace,

    [0x14] = eKeyLControl,
    [0x11] = eKeyLAlt,
    [0x12] = eKeyLShift,
    [0x0D] = eKeyTab,
    [0x66] = eKeyDelete,
    [0x59] = eKeyRShift,
    [0xE0] = eKeyRAlt,
    [0x5A] = eKeyReturn,
    [0x58] = eKeyCapital,

    [0x4A] = eKeyDivide,
    [0x76] = eKeyEscape,
    [0x49] = eKeyPeriod,

    [0x16] = eKey1,
    [0x1E] = eKey2,
    [0x26] = eKey3,
    [0x25] = eKey4,
    [0x2E] = eKey5,
    [0x36] = eKey6,
    [0x3D] = eKey7,
    [0x3E] = eKey8,
    [0x46] = eKey9,
    [0x45] = eKey0,
};

void hid::InitPs2HidStream(km::NotificationStream *stream) {
    gStream = stream;
    gHidPs2Topic = gStream->createTopic(kHidEvent, "HID.PS2");
}

km::Topic *hid::GetHidPs2Topic() {
    return gHidPs2Topic;
}

struct KeyboardState {
    bool released = false;
    bool shift = false;
};

static KeyboardState gKeyboardState;

const km::IsrEntry *hid::InstallPs2KeyboardIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::LocalIsrTable *ist) {
    gController = controller;

    const km::IsrEntry *keyboardInt = ist->allocate([](km::IsrContext *ctx) noexcept [[clang::reentrant]] -> km::IsrContext {
        km::IApic *apic = km::GetCpuLocalApic();
        defer { apic->eoi(); };

        uint8_t scancode = gController.read();
        if (scancode == 0xF0) {
            gKeyboardState.released = true;
            return *ctx;
        }

        gController.flush();

        OsKey key = kScanMap[scancode];
        if (key == eKeyUnknown) {
            KmDebugMessage("[PS2] Unknown scancode: ", km::Hex(ctx->vector), " = ", km::Hex(scancode), "\n");
        }

        if (key == eKeyLShift || key == eKeyRShift) {
            gKeyboardState.shift = !gKeyboardState.released;
        }

        OsHidKeyEvent body {
            .Key = key,
        };

        if (gKeyboardState.shift) {
            body.Modifiers |= eModifierShift;
        }

        OsHidEvent event {
            .Type = OsHidEventType(gKeyboardState.released ? eOsHidEventKeyUp : eOsHidEventKeyDown),
            .Body = { .Key = body },
        };

        gKeyboardState.released = false;

        gStream->publish<hid::HidNotification>(GetHidPs2Topic(), event);

        return *ctx;
    });

    uint8_t index = ist->index(keyboardInt);
    hid::InstallPs2DeviceIsr(ioApicSet, controller.keyboard(), target, index);

    KmDebugMessage("[PS2] Keyboard ISR: ", index, "\n");

    return keyboardInt;
}

const km::IsrEntry *hid::InstallPs2MouseIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::LocalIsrTable *ist) {
    gController = controller;

    const km::IsrEntry *mouseInt = ist->allocate([](km::IsrContext *ctx) noexcept [[clang::reentrant]] -> km::IsrContext {
        uint8_t code = gController.read();
        uint8_t x = gController.read();
        uint8_t y = gController.read();

        gController.flush();

        km::IApic *apic = km::GetCpuLocalApic();
        defer { apic->eoi(); };

        KmDebugMessage("[PS2] Mouse code: ", km::Hex(ctx->vector), " = ", km::Hex(code), " (", x, ", ", y, ")\n");

        return *ctx;
    });

    uint8_t index = ist->index(mouseInt);
    hid::InstallPs2DeviceIsr(ioApicSet, controller.mouse(), target, index);

    KmDebugMessage("[PS2] Mouse ISR: ", index, "\n");

    return mouseInt;
}
