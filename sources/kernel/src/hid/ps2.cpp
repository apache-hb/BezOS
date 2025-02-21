#include "hid/ps2.hpp"

#include "delay.hpp"
#include "log.hpp"
#include "isr/isr.hpp"

#include <emmintrin.h>

static constexpr uint16_t kData = 0x60;
static constexpr uint16_t kCommand = 0x64;

static constexpr uint8_t kInputFull = (1 << 0);
static constexpr uint8_t kOutputFull = (1 << 1);

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

static bool InputBufferFull() {
    return KmReadByte(kCommand) & kInputFull;
}

static bool OutputBufferFull() {
    return KmReadByte(kCommand) & kOutputFull;
}

static void FlushData() {
    while (InputBufferFull()) {
        KmReadByte(kData);
    }
}

static bool WaitUntilOutputEmpty(uint8_t timeout = 100) {
    while (OutputBufferFull()) {
        if (timeout-- == 0) {
            return false;
        }

        _mm_pause();
    }

    return true;
}

static bool WaitUntilInputFull(uint8_t timeout = 100) {
    while (!InputBufferFull()) {
        if (timeout-- == 0) {
            return false;
        }

        _mm_pause();
    }

    return true;
}

static OsStatus WritePort(uint8_t port, uint8_t data) {
    if (!WaitUntilOutputEmpty())
        return OsStatusTimeout;

    KmWriteByte(port, data);
    return OsStatusSuccess;
}

static OsStatus SendCommand(uint8_t command) {
    FlushData();

    return WritePort(kCommand, command);
}

static OsStatus SendDataPort1(uint8_t data) {
    FlushData();
    return WritePort(kData, data);
}

static OsStatus SendDataPort2(uint8_t data) {
    if (OsStatus status = SendCommand(kWriteSecond)) {
        return status;
    }

    return WritePort(kData, data);
}

static OsStatus SendCommandData(uint8_t command, uint8_t data) {
    if (OsStatus status = SendCommand(command)) {
        return status;
    }

    return WritePort(kData, data);
}

static OsStatus ReadDataChecked(uint8_t *data [[gnu::nonnull]]) {
    if (!WaitUntilInputFull())
        return OsStatusTimeout;

    *data = KmReadByte(kData);
    return OsStatusSuccess;
}

static OsStatus ReadCommandChecked(uint8_t command, uint8_t *data [[gnu::nonnull]]) {
    if (OsStatus status = SendCommand(command)) {
        return status;
    }

    return ReadDataChecked(data);
}

static uint8_t ReadData() {
    if (!WaitUntilInputFull())
        return 0x00;

    return KmReadByte(kData);
}

static uint8_t ReadCommand(uint8_t command) {
    SendCommand(command);

    if (!WaitUntilInputFull())
        return 0x00;

    return KmReadByte(kData);
}

static OsStatus SetupFirstChannel() {
    uint8_t config = 0;
    if (OsStatus status = ReadCommandChecked(kReadConfig, &config)) {
        return status;
    }

    // Disable interrupts and translation
    config &= ~(kFirstInt | kSecondInt | kIbmPcCompat);

    // Clear the reserved bits
    config &= ~(1 << 3 | 1 << 7);

    // Disable clock
    config |= kFirstClock;

    return SendCommandData(kWriteConfig, config);
}

static bool SelfTestController() {
    uint8_t test = 0;
    if (OsStatus status = ReadCommandChecked(kSelfTest, &test)) {
        KmDebugMessage("[PS2] PS/2 Controller communication failed: ", status, "\n");
        return false;
    }

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
    auto readData = [&](uint8_t *data) -> OsStatus {
        if (OsStatus status = ReadDataChecked(data)) {
            KmDebugMessage("[PS2] PS/2 Controller communication on ", channel, " failed: ", status, "\n");
            return status;
        }

        return OsStatusSuccess;
    };

    uint8_t b0 = 0;
    uint8_t b1 = 0;

    if (readData(&b0) != OsStatusSuccess) {
        return hid::Ps2DeviceType::eDisabled;
    }

    if (readData(&b1) != OsStatusSuccess) {
        return hid::Ps2DeviceType::eDisabled;
    }

    if (!(b0 == kDeviceOk && b1 == kAck) && !(b0 == kAck && b1 == kDeviceOk)) {
        KmDebugMessage("[PS2] PS/2 Controller ", channel, " reset failed: ", km::Hex(b0).pad(2, '0'), " ", km::Hex(b1).pad(2, '0'), "\n");
        return hid::Ps2DeviceType::eDisabled;
    }

    if (!WaitUntilInputFull()) {
        return hid::Ps2DeviceType::eAtKeyboard;
    }

    uint8_t b2 = 0;
    uint8_t b3 = 0;

    if (readData(&b2) != OsStatusSuccess) {
        return hid::Ps2DeviceType::eDisabled;
    }

    if (readData(&b3) != OsStatusSuccess) {
        return hid::Ps2DeviceType::eDisabled;
    }

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

    if (OsStatus status = SetupFirstChannel()) {
        KmDebugMessage("[PS2] PS/2 Controller first channel setup failed. ", status, "\n");

        return Ps2ControllerResult { Ps2Controller(), Ps2ControllerStatus::eControllerSelfTestFailed };
    }

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

    KmDebugMessage("[PS2] PS/2 Controller channel 1: ", device1.type(), "\n");
    KmDebugMessage("[PS2] PS/2 Controller channel 2: ", device2.type(), "\n");

    return Ps2ControllerResult { Ps2Controller(device1, device2), Ps2ControllerStatus::eOk };
}

uint8_t hid::Ps2Controller::read() const {
    if (!WaitUntilInputFull())
        return 0x00;

    return ReadData();
}

void hid::Ps2Controller::flush() {
    FlushData();
}

void hid::Ps2Controller::setMouseSampleRate(uint8_t rate) {
    hid::Ps2Device device = mouse();
    if (!device.valid()) {
        return;
    }

    device.write(0xF3);
    device.write(rate);
}

void hid::Ps2Controller::setMouseResolution(uint8_t resolution) {
    hid::Ps2Device device = mouse();
    if (!device.valid()) {
        return;
    }

    device.write(0xE8);
    device.write(resolution);
}

void hid::Ps2Controller::enableIrqs(bool first, bool second) {
    uint8_t config = ReadCommand(kReadConfig);

    KmDebugMessage("[PS2] PS/2 Controller IRQs: ", km::present(first), " ", km::present(second), " (", km::Hex(config), ")\n");

    if (first) {
        config |= kFirstInt;
    } else {
        config &= ~kFirstInt;
    }

    if (second) {
        config |= kSecondInt;
    } else {
        config &= ~kSecondInt;
    }

    SendCommandData(kWriteConfig, config);
}

void hid::Ps2Device::enable() {
    mWrite(kEnableScanning);
}

void hid::Ps2Device::disable() {
    mWrite(kDisableScanning);
}

void hid::InstallPs2DeviceIsr(km::IoApicSet& ioApicSet, const Ps2Device& device, const km::IApic *target, uint8_t isr) {
    uint8_t irq = device.isKeyboard() ? km::irq::kKeyboard : km::irq::kMouse;
    km::apic::IvtConfig config {
        .vector = isr,
        .enabled = true,
    };
    ioApicSet.setLegacyRedirect(config, irq, target);
}

using StatusFormat = km::Format<hid::Ps2ControllerStatus>;
using DeviceFormat = km::Format<hid::Ps2DeviceType>;

using namespace stdx::literals;

StatusFormat::String StatusFormat::toString(hid::Ps2ControllerStatus status) {
    switch (status) {
    case hid::Ps2ControllerStatus::eOk:
        return "Ok";
    case hid::Ps2ControllerStatus::eControllerSelfTestFailed:
        return "8042 Self Test Failed";
    case hid::Ps2ControllerStatus::ePortTestFailed:
        return "Port Test Failed";
    }
}

void DeviceFormat::format(km::IOutStream& out, hid::Ps2DeviceType type) {
    switch (type) {
    case hid::Ps2DeviceType::eDisabled:
        out.write("Disabled");
        break;
    case hid::Ps2DeviceType::eAtKeyboard:
        out.write("AT Keyboard");
        break;
    case hid::Ps2DeviceType::eMf2Keyboard:
        out.write("MF2 Keyboard");
        break;
    case hid::Ps2DeviceType::eShortKeyboard:
        out.write("Short Keyboard");
        break;
    case hid::Ps2DeviceType::eN97Keyboard:
        out.write("N97 Keyboard");
        break;
    case hid::Ps2DeviceType::e122KeyKeyboard:
        out.write("122 Key Keyboard");
        break;
    case hid::Ps2DeviceType::eJapaneseKeyboardG:
        out.write("Japanese Keyboard G");
        break;
    case hid::Ps2DeviceType::eJapaneseKeyboardP:
        out.write("Japanese Keyboard P");
        break;
    case hid::Ps2DeviceType::eJapaneseKeyboardA:
        out.write("Japanese Keyboard A");
        break;
    case hid::Ps2DeviceType::eSunKeyboard:
        out.write("Sun Keyboard");
        break;
    case hid::Ps2DeviceType::eMouse:
        out.write("Mouse");
        break;
    case hid::Ps2DeviceType::eMouseWithScroll:
        out.write("Mouse with Scroll Wheel");
        break;
    case hid::Ps2DeviceType::eMouse5Button:
        out.write("5 Button Mouse");
        break;
    default:
        out.write(km::Hex(std::to_underlying(type)));
        break;
    }
}
