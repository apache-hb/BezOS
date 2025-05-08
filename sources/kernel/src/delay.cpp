#include "delay.hpp"

#include "arch/intrin.hpp"

/// @brief Port I/O delay interface.
/// Some platforms require a delay before a port write executes completely.
/// There are also multiple methods of delaying port I/O depending on platform quirks
/// which this interface is intended to encapsulate.
using PortDelayCallback = void(*)() noexcept [[clang::blocking, clang::nonallocating]];

/// @brief Port delay for platforms that don't require delays.
static void NullPortDelay() noexcept [[clang::blocking, clang::nonallocating]] { }

/// @brief Port delay that functions by writing 0 to port 0x80.
/// This port is usually used to display status codes during POST.
/// Writing to it is begnin on most platforms, and it forces a port flush.
static void PostCodePortDelay() noexcept [[clang::blocking, clang::nonallocating]] {
    arch::IntrinX86_64::outbyte(0x80, 0);
}

static PortDelayCallback gPortDelay = NullPortDelay;

void KmSetPortDelayMethod(x64::PortDelay delay) noexcept [[clang::blocking, clang::nonallocating]] {
    switch (delay) {
    case x64::PortDelay::eNone:
        gPortDelay = NullPortDelay;
        break;
    case x64::PortDelay::ePostCode:
        gPortDelay = PostCodePortDelay;
        break;
    }
}

void KmPortDelay() noexcept [[clang::blocking, clang::nonallocating]] {
    gPortDelay();
}

// byte port io

uint8_t KmReadByte(uint16_t port) noexcept [[clang::blocking, clang::nonallocating]] {
    return arch::IntrinX86_64::inbyte(port);
}

void KmWriteByte(uint16_t port, uint8_t value) noexcept [[clang::blocking, clang::nonallocating]] {
    KmWriteByteNoDelay(port, value);
    KmPortDelay();
}

void KmWriteByteNoDelay(uint16_t port, uint8_t value) noexcept [[clang::blocking, clang::nonallocating]] {
    arch::IntrinX86_64::outbyte(port, value);
}

// word port io

uint16_t KmReadWord(uint16_t port) noexcept [[clang::blocking, clang::nonallocating]] {
    return arch::IntrinX86_64::inword(port);
}

void KmWriteWord(uint16_t port, uint16_t value) noexcept [[clang::blocking, clang::nonallocating]] {
    KmWriteWordNoDelay(port, value);
    KmPortDelay();
}

void KmWriteWordNoDelay(uint16_t port, uint16_t value) noexcept [[clang::blocking, clang::nonallocating]] {
    arch::IntrinX86_64::outword(port, value);
}

// dword port io

uint32_t KmReadLong(uint16_t port) noexcept [[clang::blocking, clang::nonallocating]] {
    return arch::IntrinX86_64::indword(port);
}

void KmWriteLong(uint16_t port, uint32_t value) noexcept [[clang::blocking, clang::nonallocating]] {
    KmWriteLongNoDelay(port, value);
    KmPortDelay();
}

void KmWriteLongNoDelay(uint16_t port, uint32_t value) noexcept [[clang::blocking, clang::nonallocating]] {
    arch::IntrinX86_64::outdword(port, value);
}
