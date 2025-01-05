#include "hid/ps2.hpp"

#include "delay.hpp"
#include "kernel.hpp"

static constexpr uint16_t kData = 0x60;
static constexpr uint16_t kCommand = 0x64;

static constexpr uint8_t kInputAvailable = (1 << 0);

static constexpr uint8_t kSelfTest = 0xAA;
static constexpr uint8_t kDisableChannel1 = 0xAD;
static constexpr uint8_t kDisableChannel2 = 0xA7;
static constexpr uint8_t kEnableChannel1 = 0xAE;
static constexpr uint8_t kEnableChannel2 = 0xA8;
static constexpr uint8_t kTestChannel1 = 0xAB;
static constexpr uint8_t kTestChannel2 = 0xA9;
static constexpr uint8_t kReadConfig = 0x20;
static constexpr uint8_t kWriteConfig = 0x60;

static constexpr uint8_t kSelfTestOk = 0x55;

static constexpr uint8_t kFirstInt = (1 << 0);
static constexpr uint8_t kSecondInt = (1 << 1);
static constexpr uint8_t kFirstClock = (1 << 4);
static constexpr uint8_t kSecondClock = (1 << 5);
static constexpr uint8_t kIbmPcCompat = (1 << 6);

static void FlushData() {
    while (KmReadByte(kCommand) & kInputAvailable) {
        KmReadByte(kData);
    }
}

static void SendCommand(uint8_t command) {
    KmWriteByte(kCommand, command);
}

static void SendCommandData(uint8_t command, uint8_t data) {
    SendCommand(command);
    KmWriteByte(kData, data);
}

static uint8_t ReadCommand(uint8_t command) {
    KmWriteByte(kCommand, command);

    while (!(KmReadByte(kCommand) & kInputAvailable)) { }

    return KmReadByte(kData);
}

static void SetupFirstChannel() {
    uint8_t config = ReadCommand(kReadConfig);

    // Disable interrupts and translation
    config &= ~(kFirstInt | kIbmPcCompat);

    // Clear the reserved bits
    config &= ~(1 << 3 | 1 << 7);

    // Enable clock
    config |= kFirstClock;

    SendCommandData(kWriteConfig, config);
}

static bool SelfTestController() {
    uint8_t test = ReadCommand(kSelfTest);
    if (test != kSelfTestOk) {
        KmDebugMessage("[PS2] PS/2 Controller self test failed: ", km::Hex(kSelfTestOk).pad(2, '0'), " != ", km::Hex(test).pad(2, '0'), "\n");
        return false;
    }

    return true;
}

static bool TestSecondChannel() {
    SendCommand(kEnableChannel2);

    uint8_t config = ReadCommand(kReadConfig);
    if (config & kSecondClock) {
        return false;
    }

    SendCommand(kDisableChannel2);

    config &= ~(kSecondClock | kSecondInt);

    SendCommandData(kWriteConfig, config);

    return true;
}

static void TestPorts(bool *dualChannel [[gnu::nonnull]], bool *firstChannel [[gnu::nonnull]]) {
    uint8_t c1 = ReadCommand(kTestChannel1);
    if (c1 != 0) {
        KmDebugMessage("[PS2] PS/2 Controller first port test failed: ", km::Hex(0).pad(2, '0'), " != ", km::Hex(c1).pad(2, '0'), "\n");
        *firstChannel = false;
    }

    if (*dualChannel) {
        uint8_t c2 = ReadCommand(kTestChannel2);
        if (c2 != 0) {
            KmDebugMessage("[PS2] PS/2 Controller second port test failed: ", km::Hex(0).pad(2, '0'), " != ", km::Hex(c2).pad(2, '0'), "\n");
            *dualChannel = false;
        }
    }
}

hid::Ps2ControllerResult hid::EnablePs2Controller() {
    SendCommand(kDisableChannel1);
    SendCommand(kDisableChannel2);

    FlushData();

    SetupFirstChannel();

    if (!SelfTestController()) {
        return Ps2ControllerResult { Ps2Controller(false, false), Ps2ControllerStatus::eControllerSelfTestFailed };
    }

    bool channel1 = true;
    bool channel2 = TestSecondChannel();

    TestPorts(&channel2, &channel1);

    if (!channel1 && !channel2) {
        return Ps2ControllerResult { Ps2Controller(false, false), Ps2ControllerStatus::ePortTestFailed };
    }

    if (channel1) {
        SendCommand(kEnableChannel1);
    }

    if (channel2) {
        SendCommand(kEnableChannel2);
    }

    return Ps2ControllerResult { Ps2Controller(channel1, channel2), Ps2ControllerStatus::eOk };
}

using StatusFormat = km::StaticFormat<hid::Ps2ControllerStatus>;

using namespace stdx::literals;

StatusFormat::String StatusFormat::toString(hid::Ps2ControllerStatus status) {
    switch (status) {
    case hid::Ps2ControllerStatus::eOk:
        return "Ok"_sv;
    case hid::Ps2ControllerStatus::eControllerSelfTestFailed:
        return "8042 Self Test Failed"_sv;
    case hid::Ps2ControllerStatus::ePortTestFailed:
        return "Port Test Failed"_sv;
    }
}
