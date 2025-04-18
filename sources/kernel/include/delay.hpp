#pragma once

#include "common/util/util.hpp"

#include <stdint.h>

namespace x64 {
    enum class PortDelay {
        eNone,
        ePostCode,
    };
}

void KmSetPortDelayMethod(x64::PortDelay delay);

WEAK_SYMBOL_TEST
void KmPortDelay(void);

void KmWriteByte(uint16_t port, uint8_t value);

WEAK_SYMBOL_TEST
uint8_t KmReadByte(uint16_t port);

WEAK_SYMBOL_TEST
void KmWriteByteNoDelay(uint16_t port, uint8_t value);

void KmWriteWord(uint16_t port, uint16_t value);

WEAK_SYMBOL_TEST
uint16_t KmReadWord(uint16_t port);

WEAK_SYMBOL_TEST
void KmWriteWordNoDelay(uint16_t port, uint16_t value);

void KmWriteLong(uint16_t port, uint32_t value);

WEAK_SYMBOL_TEST
uint32_t KmReadLong(uint16_t port);

WEAK_SYMBOL_TEST
void KmWriteLongNoDelay(uint16_t port, uint32_t value);
