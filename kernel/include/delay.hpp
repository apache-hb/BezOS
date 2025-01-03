#pragma once

#include <stdint.h>

namespace x64 {
    enum class PortDelay {
        eNone,
        ePostCode,
    };
}

void KmSetPortDelayMethod(x64::PortDelay delay);
void KmPortDelay(void);

uint8_t KmReadByte(uint16_t port);
void KmWriteByte(uint16_t port, uint8_t value);
void KmWriteByteNoDelay(uint16_t port, uint8_t value);

uint16_t KmReadWord(uint16_t port);
void KmWriteWord(uint16_t port, uint16_t value);
void KmWriteWordNoDelay(uint16_t port, uint16_t value);

uint32_t KmReadDword(uint16_t port);
void KmWriteDword(uint16_t port, uint32_t value);
void KmWriteDwordNoDelay(uint16_t port, uint32_t value);
