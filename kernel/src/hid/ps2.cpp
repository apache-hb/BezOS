#include "hid/ps2.hpp"

#include "delay.hpp"
#include "kernel.hpp"

static constexpr uint16_t kData = 0x60;
static constexpr uint16_t kCommand = 0x64;

static constexpr uint8_t kInputAvailable = (1 << 0);

static uint8_t SendCommand(uint8_t command) {
    KmWriteByte(kCommand, command);

    while (!(KmReadByte(kCommand) & kInputAvailable)) { }

    return KmReadByte(kData);
}

hid::OpenPs2ControllerResult hid::OpenPs2Controller() {
    static constexpr uint8_t kSelfTest = 0xAA;
    uint8_t test = SendCommand(kSelfTest);
    if (test != 0xAA) {
        KmDebugMessage("[PS2] PS/2 Controller self test failed: ", kSelfTest, " != ", test, "\n");
        return OpenPs2ControllerResult { Ps2Controller(), Ps2ControllerStatus::eSelfTestFailed };
    }

    return OpenPs2ControllerResult { Ps2Controller(), Ps2ControllerStatus::eOk };
}
