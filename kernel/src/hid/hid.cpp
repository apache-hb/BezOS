#include "hid/hid.hpp"

#include "log.hpp"

static hid::Ps2Controller gController;
static km::NotificationStream *gStream;

const km::IsrTable::Entry *hid::InstallPs2KeyboardIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::IsrTable *ist, km::NotificationStream *stream) {
    gController = controller;
    gStream = stream;

    const km::IsrTable::Entry *keyboardInt = ist->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        uint8_t scancode = gController.read();
        gController.flush();

        KmDebugMessage("[PS2] Keyboard scancode: ", km::Hex(ctx->vector), " = ", km::Hex(scancode), "\n");
        km::IApic *apic = km::GetCpuLocalApic();
        apic->eoi();
        return *ctx;
    });

    hid::InstallPs2DeviceIsr(ioApicSet, controller.keyboard(), target, ist->index(keyboardInt));

    return keyboardInt;
}

const km::IsrTable::Entry *hid::InstallPs2MouseIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::IsrTable *ist, km::NotificationStream *stream) {
    gController = controller;
    gStream = stream;

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
