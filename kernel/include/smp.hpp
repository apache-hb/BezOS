#pragma once

#include "apic.hpp"

#include "memory.hpp"

void KmInitSmp(km::SystemMemory& memory, km::LocalAPIC& bsp, acpi::AcpiTables& acpiTables);
