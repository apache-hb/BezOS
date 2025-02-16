#pragma once

#include <stdint.h>

enum {
    eKeyUnknown = 0x00,

    eKeyLButton   = 0x01,
    eKeyRButton   = 0x02,
    eKeyCancel    = 0x03,
    eKeyMButton   = 0x04,

    eKeyXButton1  = 0x05,
    eKeyXButton2  = 0x06,

    eKeyBack      = 0x08,
    eKeyTab       = 0x09,

    eKeyClear     = 0x0C,
    eKeyReturn    = 0x0D,

    eKeyShift     = 0x10,
    eKeyControl   = 0x11,
    eKeyMenu      = 0x12,
    eKeyPause     = 0x13,
    eKeyCapital   = 0x14,

    eKeyEscape    = 0x1B,

    eKeySpace     = 0x20,
    eKeyPrior     = 0x21,
    eKeyNext      = 0x22,
    eKeyEnd       = 0x23,
    eKeyHome      = 0x24,
    eKeyLeft      = 0x25,
    eKeyUp        = 0x26,
    eKeyRight     = 0x27,
    eKeyDown      = 0x28,

    eKeyInsert    = 0x2D,
    eKeyDelete    = 0x2E,

    eKeyNumpad0   = 0x60,
    eKeyNumpad1   = 0x61,
    eKeyNumpad2   = 0x62,
    eKeyNumpad3   = 0x63,
    eKeyNumpad4   = 0x64,
    eKeyNumpad5   = 0x65,
    eKeyNumpad6   = 0x66,
    eKeyNumpad7   = 0x67,
    eKeyNumpad8   = 0x68,
    eKeyNumpad9   = 0x69,
    eKeyMultiply  = 0x6A,
    eKeyAdd       = 0x6B,
    eKeySeparator = 0x6C,
    eKeySubtract  = 0x6D,
    eKeyDecimal   = 0x6E,
    eKeyDivide    = 0x6F,

    eKeyLShift    = 0xA0,
    eKeyRShift    = 0xA1,
    eKeyLControl  = 0xA2,
    eKeyRControl  = 0xA3,
    eKeyLAlt      = 0xA4,
    eKeyRAlt      = 0xA5,

    eKeyA         = 'A',
    eKeyB         = 'B',
    eKeyC         = 'C',
    eKeyD         = 'D',
    eKeyE         = 'E',
    eKeyF         = 'F',
    eKeyG         = 'G',
    eKeyH         = 'H',
    eKeyI         = 'I',
    eKeyJ         = 'J',
    eKeyK         = 'K',
    eKeyL         = 'L',
    eKeyM         = 'M',
    eKeyN         = 'N',
    eKeyO         = 'O',
    eKeyP         = 'P',
    eKeyQ         = 'Q',
    eKeyR         = 'R',
    eKeyS         = 'S',
    eKeyT         = 'T',
    eKeyU         = 'U',
    eKeyV         = 'V',
    eKeyW         = 'W',
    eKeyX         = 'X',
    eKeyY         = 'Y',
    eKeyZ         = 'Z',

    eKey0         = '0',
    eKey1         = '1',
    eKey2         = '2',
    eKey3         = '3',
    eKey4         = '4',
    eKey5         = '5',
    eKey6         = '6',
    eKey7         = '7',
    eKey8         = '8',
    eKey9         = '9'
};

typedef uint8_t OsKey;
