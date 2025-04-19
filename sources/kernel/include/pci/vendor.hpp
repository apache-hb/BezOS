#pragma once

#include "util/format.hpp"

#include <stdint.h>

namespace pci {
    enum class VendorId : uint16_t {
        eInvalid = 0xFFFF,

        eIntel = 0x8086,

        eAMD = 0x1022,
        eATI = 0x1002,

        eMicron = 0x1344,

        eNvidia = 0x10DE,

        eRealtek = 0x10EC,
        eMediaTek = 0x14C3,

        eQemuVirtio = 0x1af4,
        eQemu = 0x1b36,
        eQemuVga = 0x1234,

        eSandisk = 0x15B7,

        eOracle = 0x108e,
        eBroadcom = 0x1166,
    };
}

template<>
struct km::Format<pci::VendorId> {
    using String = stdx::StaticString<64>;
    static String toString(pci::VendorId id);
};
