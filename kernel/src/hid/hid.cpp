#include "hid/hid.hpp"

#include "log.hpp"

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

const km::IsrTable::Entry *hid::InstallPs2KeyboardIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::IsrTable *ist) {
    gController = controller;

    const km::IsrTable::Entry *keyboardInt = ist->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        uint8_t scancode = 0;
        bool pressed = true;

        uint8_t segment = gController.read();
        if (segment == 0xF0) {
            pressed = false;
            scancode = gController.read();
        } else {
            pressed = true;
            scancode = segment;
        }

        gController.flush();

        OsKey key = kScanMap[scancode];
        if (key == eKeyUnknown) {
            KmDebugMessage("[PS2] Unknown scancode: ", km::Hex(ctx->vector), " = ", km::Hex(scancode), "\n");
        }

        hid::HidEvent event {
            .type = pressed ? hid::HidEventType::eKeyDown : hid::HidEventType::eKeyUp,
            .key = {
                .code = key,
            },
        };

        gStream->publish<hid::HidNotification>(GetHidPs2Topic(), event);

        km::IApic *apic = km::GetCpuLocalApic();
        apic->eoi();
        return *ctx;
    });

    hid::InstallPs2DeviceIsr(ioApicSet, controller.keyboard(), target, ist->index(keyboardInt));

    return keyboardInt;
}

const km::IsrTable::Entry *hid::InstallPs2MouseIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::IsrTable *ist) {
    gController = controller;

    const km::IsrTable::Entry *mouseInt = ist->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        uint8_t code = gController.read();
        uint8_t x = gController.read();
        uint8_t y = gController.read();

        gController.flush();

        km::IApic *apic = km::GetCpuLocalApic();
        apic->eoi();

        KmDebugMessage("[PS2] Mouse code: ", km::Hex(ctx->vector), " = ", km::Hex(code), " (", x, ", ", y, ")\n");

        return *ctx;
    });

    hid::InstallPs2DeviceIsr(ioApicSet, controller.mouse(), target, ist->index(mouseInt));

    return mouseInt;
}
