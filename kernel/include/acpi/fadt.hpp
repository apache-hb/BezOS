#pragma once

#include "acpi/header.hpp"
#include "util/format.hpp"
#include "util/util.hpp"

namespace acpi {
    enum class IapcBootArch : uint16_t {
        eLegacyDevices = (1 << 0),
        e8042Controller = (1 << 1),
        eVgaNotPresent = (1 << 2),
        eMsiNotPresent = (1 << 3),
    };

    UTIL_BITFLAGS(IapcBootArch);

    struct [[gnu::packed]] Fadt {
        RsdtHeader header; // signature must be "FACP"

        uint32_t firmwareCtrl;
        uint32_t dsdt;
        uint8_t reserved0[1];
        uint8_t preferredPmProfile;
        uint16_t sciInt;
        uint32_t smiCmd;
        uint8_t acpiEnable;
        uint8_t acpiDisable;
        uint8_t s4BiosReq;
        uint8_t pstateCnt;

        uint32_t pm1aEvtBlk;
        uint32_t pm1bEvtBlk;
        uint32_t pm1aCntBlk;
        uint32_t pm1bCntBlk;
        uint32_t pm2CntBlk;
        uint32_t pmTmrBlk;
        uint32_t gpe0Blk;
        uint32_t gpe1Blk;

        uint8_t pm1EvtLen;
        uint8_t pm1CntLen;
        uint8_t pm2CntLen;
        uint8_t pmTmrLen;
        uint8_t gpe0BlkLen;
        uint8_t gpe1BlkLen;
        uint8_t gpe1Base;
        uint8_t cstateCtl;
        uint16_t worstC2Latency;
        uint16_t worstC3Latency;
        uint16_t flushSize;
        uint16_t flushStride;
        uint8_t dutyOffset;
        uint8_t dutyWidth;
        uint8_t dayAlrm;
        uint8_t monAlrm;
        uint8_t century;
        IapcBootArch iapcBootArch;
        uint8_t reserved1[1];
        uint32_t flags;
        GenericAddress resetReg;
        uint8_t resetValue;
        uint16_t armBootArch;
        uint8_t fadtMinorVersion;
        uint64_t x_firmwareCtrl;
        uint64_t x_dsdt;

        GenericAddress x_pm1aEvtBlk;
        GenericAddress x_pm1bEvtBlk;
        GenericAddress x_pm1aCntBlk;
        GenericAddress x_pm1bCntBlk;
        GenericAddress x_pm2CntBlk;
        GenericAddress x_pmTmrBlk;
        GenericAddress x_gpe0Blk;
        GenericAddress x_gpe1Blk;
        GenericAddress sleepControlReg;
        GenericAddress sleepStatusReg;
        uint64_t hypervisorVendor;
    };

    static_assert(sizeof(Fadt) == 276);
}

template<>
struct km::Format<acpi::IapcBootArch> {
    using String = stdx::StaticString<64>;
    static String toString(acpi::IapcBootArch arch);
};
