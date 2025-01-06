#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "kernel/test/ports.hpp"

#include "uart.hpp"

using namespace km::uart::detail;

class MockSerial : public kmtest::IPeripheral {
    uint16_t mBase;

public:
    MockSerial(uint16_t base)
        : IPeripheral(std::format("UART {:#x}", base))
        , mBase(base)
    { }

    MOCK_METHOD(void, write8, (uint16_t port, uint8_t value), (override));
    MOCK_METHOD(uint8_t, read8, (uint16_t port), (override));

    void connect(kmtest::PeripheralRegistry& device) override {
        device.wire(mBase + kData, this);
        device.wire(mBase + kInterruptEnable, this);
        device.wire(mBase + kFifoControl, this);
        device.wire(mBase + kLineControl, this);
        device.wire(mBase + kModemControl, this);
        device.wire(mBase + kLineStatus, this);
        device.wire(mBase + KModemStatus, this);
        device.wire(mBase + kScratch, this);
    }
};

using testing::_;

static void SerialConfigLine(testing::StrictMock<MockSerial>& serial, uint16_t base) {
    // interrupts need to be disabled
    EXPECT_CALL(serial, write8(base + kInterruptEnable, 0x00));

    // the DLAB needs to be enabled
    EXPECT_CALL(serial, write8(base + kLineControl, (1 << kDlabOffset)));

    // A divisor of 12 should be written for a baud rate of 9600
    EXPECT_CALL(serial, write8(base + 0, 12));
    EXPECT_CALL(serial, write8(base + 1, 0));

    // 8 bit, one stop bit, no parity
    EXPECT_CALL(serial, write8(base + kLineControl, 0b11));

    // enable fifo, clear receive and transmit fifo, 1 byte threshold
    EXPECT_CALL(serial, write8(base + kFifoControl, (1 << 0) | (1 << 1) | (1 << 2) | (0b11 << 6)));

    // enable irqs, rts/dtr set
    EXPECT_CALL(serial, write8(base + kModemControl, 0x0F));
}

static void PassScratchTest(testing::StrictMock<MockSerial>& serial, uint16_t base, uint8_t& scratch) {
    // scratch test
    EXPECT_CALL(serial, write8(base + kScratch, _))
        .WillOnce(testing::SaveArg<1>(&scratch));

    EXPECT_CALL(serial, read8(base + kScratch))
        .WillOnce(testing::ReturnPointee(&scratch));
}

TEST(SerialTest, OpenSerialOk) {
    testing::StrictMock<MockSerial> serial(km::com::kComPort1);
    kmtest::devices().add(&serial);

    uint8_t loopback;
    uint8_t scratch;

    {
        testing::InSequence s;

        PassScratchTest(serial, km::com::kComPort1, scratch);

        SerialConfigLine(serial, km::com::kComPort1);

        // enable loopback
        EXPECT_CALL(serial, write8(km::com::kComPort1 + kModemControl, 0x1E));

        // successful loopback test
        EXPECT_CALL(serial, write8(km::com::kComPort1 + kData, _))
            .WillOnce(testing::SaveArg<1>(&loopback));

        EXPECT_CALL(serial, read8(km::com::kComPort1 + kData))
            .WillOnce(testing::ReturnPointee(&loopback));

        // disable loopback
        EXPECT_CALL(serial, write8(km::com::kComPort1 + kModemControl, 0x0F));
    }

    km::ComPortInfo info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
        .skipLoopbackTest = false,
    };

    km::OpenSerialResult result = km::OpenSerial(info);

    EXPECT_EQ(result.status, km::SerialPortStatus::eOk);
}

TEST(SerialTest, OpenSerialScratchFail) {
    testing::StrictMock<MockSerial> serial(km::com::kComPort1);
    kmtest::devices().add(&serial);

    uint8_t scratch;

    {
        testing::InSequence s;

        // if the scratch test fails nothing else should be done
        EXPECT_CALL(serial, write8(km::com::kComPort1 + kScratch, _))
            .WillOnce(testing::SaveArg<1>(&scratch));

        EXPECT_CALL(serial, read8(km::com::kComPort1 + kScratch))
            .WillOnce(testing::Invoke([scratch](uint16_t) -> uint8_t {
                return scratch + 1;
            }));
    }

    km::ComPortInfo info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
        .skipLoopbackTest = false,
    };

    km::OpenSerialResult result = km::OpenSerial(info);

    EXPECT_EQ(result.status, km::SerialPortStatus::eScratchTestFailed);
}

TEST(SerialTest, OpenSerialLoopbackFail) {
    testing::StrictMock<MockSerial> serial(km::com::kComPort1);
    kmtest::devices().add(&serial);

    uint8_t scratch;
    uint8_t loopback;

    {
        testing::InSequence s;

        PassScratchTest(serial, km::com::kComPort1, scratch);

        SerialConfigLine(serial, km::com::kComPort1);

        // failed loopback test
        EXPECT_CALL(serial, write8(km::com::kComPort1 + kModemControl, 0x1E));

        EXPECT_CALL(serial, write8(km::com::kComPort1 + kData, _))
            .WillOnce(testing::SaveArg<1>(&loopback));

        EXPECT_CALL(serial, read8(km::com::kComPort1 + kData))
            .WillOnce(testing::Invoke([loopback](uint16_t) -> uint8_t {
                return loopback + 1;
            }));

        // disable loopback
        EXPECT_CALL(serial, write8(km::com::kComPort1 + kModemControl, 0x0F));
    }

    km::ComPortInfo info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
        .skipLoopbackTest = false,
    };

    km::OpenSerialResult result = km::OpenSerial(info);

    EXPECT_EQ(result.status, km::SerialPortStatus::eLoopbackTestFailed);
}

TEST(SerialTest, SkipLookbackTest) {
    testing::StrictMock<MockSerial> serial(km::com::kComPort1);
    kmtest::devices().add(&serial);

    uint8_t scratch;

    {
        testing::InSequence s;

        PassScratchTest(serial, km::com::kComPort1, scratch);

        SerialConfigLine(serial, km::com::kComPort1);
    }

    km::ComPortInfo info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
        .skipLoopbackTest = true,
    };

    km::OpenSerialResult result = km::OpenSerial(info);

    EXPECT_EQ(result.status, km::SerialPortStatus::eOk);
}