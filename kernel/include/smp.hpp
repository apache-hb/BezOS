#pragma once

#include "apic.hpp"

#include "memory.hpp"

void KmInitSmp(km::SystemMemory& memory, km::IIntController *bsp, acpi::AcpiTables& acpiTables, bool useX2Apic);
