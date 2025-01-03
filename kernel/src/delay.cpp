#include "delay.hpp"

#include "arch/intrin.hpp"

/// @brief Port I/O delay interface.
/// Some platforms require a delay before a port write executes completely.
/// There are also multiple methods of delaying port I/O depending on platform quirks
/// which this interface is intended to encapsulate.
using PortDelayCallback = void(*)(void);

/// @brief Port delay for platforms that don't require delays.
static void NullPortDelay(void) { }

/// @brief Port delay that functions by writing 0 to port 0x80.
/// This port is usually used to display status codes during POST.
/// Writing to it is begnin on most platforms, and it forces a port flush.
static void PostCodePortDelay(void) {
    __outbyte(0x80, 0);
}

static PortDelayCallback gPortDelay = NullPortDelay;

void KmSetPortDelayMethod(x64::PortDelay delay) {
    switch (delay) {
    case x64::PortDelay::eNone:
        gPortDelay = NullPortDelay;
        break;
    case x64::PortDelay::ePostCode:
        gPortDelay = PostCodePortDelay;
        break;
    }
}

void KmPortDelay(void) {
    gPortDelay();
}

// byte port io

uint8_t KmReadByte(uint16_t port) {
    return __inbyte(port);
}

void KmWriteByte(uint16_t port, uint8_t value) {
    __outbyte(port, value);
    KmPortDelay();
}

void KmWriteByteNoDelay(uint16_t port, uint8_t value) {
    __outbyte(port, value);
}

// word port io

uint16_t KmReadWord(uint16_t port) {
    return __inword(port);
}

void KmWriteWord(uint16_t port, uint16_t value) {
    __outword(port, value);
    KmPortDelay();
}

void KmWriteWordNoDelay(uint16_t port, uint16_t value) {
    __outword(port, value);
}

// dword port io

uint32_t KmReadDword(uint16_t port) {
    return __indword(port);
}

void KmWriteDword(uint16_t port, uint32_t value) {
    __outdword(port, value);
    KmPortDelay();
}

void KmWriteDwordNoDelay(uint16_t port, uint32_t value) {
    __outdword(port, value);
}
