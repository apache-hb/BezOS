#pragma once

#include "apic.hpp"

#include "memory.hpp"

namespace km {
    void InitSmp(km::SystemMemory& memory, km::IApic *bsp, acpi::AcpiTables& acpiTables, bool useX2Apic);
}
