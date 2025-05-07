#include <gtest/gtest.h>

#include "ports.hpp"

#include "common/compiler/compiler.hpp"

void kmtest::IPeripheral::write8(uint16_t port, uint8_t value) {
    GTEST_FAIL() << "Peripheral " << mName << " received a 8-bit write. Port: " << port << " Value: " << value;
}

void kmtest::IPeripheral::write16(uint16_t port, uint16_t value) {
    GTEST_FAIL() << "Peripheral " << mName << " received a 16-bit write. Port: " << port << " Value: " << value;
}

void kmtest::IPeripheral::write32(uint16_t port, uint32_t value) {
    GTEST_FAIL() << "Peripheral " << mName << " received a 32-bit write. Port: " << port << " Value: " << value;
}

uint8_t kmtest::IPeripheral::read8(uint16_t port) {
    (void)[&] { GTEST_FAIL() << "Peripheral " << mName << " received a 8-bit read. Port: " << port; };
    return 0;
}

uint16_t kmtest::IPeripheral::read16(uint16_t port) {
    (void)[&] { GTEST_FAIL() << "Peripheral " << mName << " received a 16-bit read. Port: " << port; };
    return 0;
}

uint32_t kmtest::IPeripheral::read32(uint16_t port) {
    (void)[&] { GTEST_FAIL() << "Peripheral " << mName << " received a 32-bit read. Port: " << port; };
    return 0;
}


void kmtest::PeripheralRegistry::write8(uint16_t port, uint8_t value) {
    if (IPeripheral *device = mPortMap[port]) {
        device->write8(port, value);
    } else if (mDefault) {
        mDefault->write8(port, value);
    } else {
        GTEST_FAIL() << "No peripheral connected to port " << port << " for 8-bit write. Value: " << value;
    }
}

void kmtest::PeripheralRegistry::write16(uint16_t port, uint16_t value) {
    if (IPeripheral *device = mPortMap[port]) {
        device->write16(port, value);
    } else if (mDefault) {
        mDefault->write16(port, value);
    } else {
        GTEST_FAIL() << "No peripheral connected to port " << port << " for 16-bit write. Value: " << value;
    }
}

void kmtest::PeripheralRegistry::write32(uint16_t port, uint32_t value) {
    if (IPeripheral *device = mPortMap[port]) {
        device->write32(port, value);
    } else if (mDefault) {
        mDefault->write32(port, value);
    } else {
        GTEST_FAIL() << "No peripheral connected to port " << port << " for 32-bit write. Value: " << value;
    }
}

uint8_t kmtest::PeripheralRegistry::read8(uint16_t port) {
    if (IPeripheral *device = mPortMap[port]) {
        return device->read8(port);
    } else if (mDefault) {
        return mDefault->read8(port);
    } else {
        [&] { GTEST_FAIL() << "No peripheral connected to port " << port << " for 8-bit read."; }();
        return 0;
    }
}

uint16_t kmtest::PeripheralRegistry::read16(uint16_t port) {
    if (IPeripheral *device = mPortMap[port]) {
        return device->read16(port);
    } else if (mDefault) {
        return mDefault->read16(port);
    } else {
        [&] { GTEST_FAIL() << "No peripheral connected to port " << port << " for 16-bit read."; }();
        return 0;
    }
}

uint32_t kmtest::PeripheralRegistry::read32(uint16_t port) {
    if (IPeripheral *device = mPortMap[port]) {
        return device->read32(port);
    } else if (mDefault) {
        return mDefault->read32(port);
    } else {
        [&] { GTEST_FAIL() << "No peripheral connected to port " << port << " for 32-bit read."; }();
        return 0;
    }
}

kmtest::PeripheralRegistry& kmtest::devices() {
    static PeripheralRegistry sCollection;
    return sCollection;
}

CLANG_DIAGNOSTIC_PUSH();
CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects"); // We don't care about realtime effects in tests

void KmPortDelay(void) noexcept {
    kmtest::devices().delay();
}

uint8_t KmReadByte(uint16_t port) noexcept {
    return kmtest::devices().read8(port);
}

void KmWriteByteNoDelay(uint16_t port, uint8_t value) noexcept {
    kmtest::devices().write8(port, value);
}

uint16_t KmReadWord(uint16_t port) noexcept {
    return kmtest::devices().read16(port);
}

void KmWriteWordNoDelay(uint16_t port, uint16_t value) noexcept {
    kmtest::devices().write16(port, value);
}

uint32_t KmReadLong(uint16_t port) noexcept {
    return kmtest::devices().read32(port);
}

void KmWriteLongNoDelay(uint16_t port, uint32_t value) noexcept  {
    kmtest::devices().write32(port, value);
}

CLANG_DIAGNOSTIC_POP();
