#pragma once

#include "apic.hpp"

#include "memory.hpp"

void KmInitSmp(km::SystemMemory& memory, km::LocalApic& bsp, acpi::AcpiTables& acpiTables);
