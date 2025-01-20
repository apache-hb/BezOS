#include "hid/ps2.hpp"

#include "delay.hpp"
#include "log.hpp"

#include <emmintrin.h>

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

static constexpr uint8_t kWriteSecond = 0xD4;

static constexpr uint8_t kSelfTestOk = 0x55;
static constexpr uint8_t kDeviceOk = 0xAA;
static constexpr uint8_t kAck = 0xFA;

static constexpr uint8_t kEnableScanning = 0xF4;
static constexpr uint8_t kDisableScanning = 0xF5;

static constexpr uint8_t kFirstInt = (1 << 0);
static constexpr uint8_t kSecondInt = (1 << 1);
static constexpr uint8_t kFirstClock = (1 << 4);
static constexpr uint8_t kSecondClock = (1 << 5);
static constexpr uint8_t kIbmPcCompat = (1 << 6);

static bool DataAvailable() {
    return KmReadByte(kCommand) & kInputAvailable;
}

static void FlushData() {
    while (DataAvailable()) {
        KmReadByte(kData);
    }
}

static void SendCommand(uint8_t command) {
    KmWriteByte(kCommand, command);
}

static void SendDataPort1(uint8_t data) {
    KmWriteByte(kData, data);
}

static void SendDataPort2(uint8_t data) {
    SendCommand(kWriteSecond);
    KmWriteByte(kData, data);
}

static void SendCommandData(uint8_t command, uint8_t data) {
    SendCommand(command);
    KmWriteByte(kData, data);
}

static bool WaitUntilDataAvailable(uint8_t timeout = 100) {
    while (!DataAvailable() && timeout-- > 0) {
        _mm_pause();
    }

    return timeout > 0;
}

static uint8_t ReadData() {
    WaitUntilDataAvailable();

    return KmReadByte(kData);
}

static uint8_t ReadCommand(uint8_t command) {
    KmWriteByte(kCommand, command);

    WaitUntilDataAvailable();

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

static hid::Ps2DeviceType CheckDeviceReset(stdx::StringView channel) {
    uint8_t b0 = ReadData();
    uint8_t b1 = ReadData();

    if (!(b0 == kDeviceOk && b1 == kAck) && !(b0 == kAck && b1 == kDeviceOk)) {
        KmDebugMessage("[PS2] PS/2 Controller ", channel, " reset failed: ", km::Hex(b0).pad(2, '0'), " ", km::Hex(b1).pad(2, '0'), "\n");
        return hid::Ps2DeviceType::eDisabled;
    }

    if (!WaitUntilDataAvailable()) {
        return hid::Ps2DeviceType::eAtKeyboard;
    }

    uint8_t b2 = ReadData();
    uint8_t b3 = ReadData();

    if (b3 == 0) {
        switch (b2) {
        case 0x00: return hid::Ps2DeviceType::eMouse;
        case 0x03: return hid::Ps2DeviceType::eMouseWithScroll;
        case 0x04: return hid::Ps2DeviceType::eMouse5Button;
        default: break;
        }
    }

    if (b3 == 0xAB || b3 == 0xAC) {
        std::swap(b2, b3);
    }

    uint16_t type
        = uint16_t(b3) << 8
        | uint16_t(b2) << 0;


    switch (type) {
    case 0xAB83: case 0xABC1:
        return hid::Ps2DeviceType::eMf2Keyboard;
    case 0xAB84:
        return hid::Ps2DeviceType::eShortKeyboard;
    case 0xAB85:
        return hid::Ps2DeviceType::eN97Keyboard;
    case 0xAB86:
    case 0xAAAA:
        return hid::Ps2DeviceType::e122KeyKeyboard;
    case 0xAB90:
        return hid::Ps2DeviceType::eJapaneseKeyboardG;
    case 0xAB91:
        return hid::Ps2DeviceType::eJapaneseKeyboardP;
    case 0xAB92:
        return hid::Ps2DeviceType::eJapaneseKeyboardA;
    case 0xACA1:
        return hid::Ps2DeviceType::eSunKeyboard;
    }

    KmDebugMessage("[PS2] PS/2 Controller ", channel, " unknown device type: ", km::Hex(type).pad(4, '0'), "\n");

    return hid::Ps2DeviceType::eDisabled;
}

hid::Ps2ControllerResult hid::EnablePs2Controller() {
    SendCommand(kDisableChannel1);
    SendCommand(kDisableChannel2);

    FlushData();

    SetupFirstChannel();

    if (!SelfTestController()) {
        return Ps2ControllerResult { Ps2Controller(), Ps2ControllerStatus::eControllerSelfTestFailed };
    }

    bool channel1 = true;
    bool channel2 = TestSecondChannel();

    TestPorts(&channel2, &channel1);

    if (!channel1 && !channel2) {
        return Ps2ControllerResult { Ps2Controller(), Ps2ControllerStatus::ePortTestFailed };
    }

    hid::Ps2DeviceType type1 = hid::Ps2DeviceType::eDisabled;
    hid::Ps2DeviceType type2 = hid::Ps2DeviceType::eDisabled;

    if (channel1) {
        SendCommand(kEnableChannel1);

        SendDataPort1(0xFF);

        type1 = CheckDeviceReset("channel 1");

        if (type1 == hid::Ps2DeviceType::eDisabled) {
            channel1 = false;
        }
    }

    if (channel2) {
        SendCommand(kEnableChannel2);

        SendDataPort2(0xFF);

        type2 = CheckDeviceReset("channel 2");

        if (type2 == hid::Ps2DeviceType::eDisabled) {
            channel2 = false;
        }
    }

    hid::Ps2Device device1 { type1, SendDataPort1 };
    hid::Ps2Device device2 { type2, SendDataPort2 };

    if (!device1.isKeyboard() && device2.isKeyboard()) {
        std::swap(device1, device2);
    }

    return Ps2ControllerResult { Ps2Controller(device1, device2), Ps2ControllerStatus::eOk };
}

uint8_t hid::Ps2Controller::read() const {
    while (!DataAvailable()) {
        _mm_pause();
    }

    return ReadData();
}

void hid::Ps2Device::enable() {
    write(kEnableScanning);
}

void hid::Ps2Device::disable() {
    write(kDisableScanning);
}

using StatusFormat = km::Format<hid::Ps2ControllerStatus>;

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
