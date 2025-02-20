#pragma once

#include "apic.hpp"

#include "isr.hpp"
#include "memory.hpp"

namespace km {
    /// @brief The stack size for each AP.
    constexpr size_t kStartupStackSize = x64::kPageSize * 4;

    /// @brief The amount of memory required to startup a single AP.
    constexpr size_t kStartupMemorySize = kStartupStackSize;

    size_t GetStartupMemorySize(const acpi::AcpiTables& acpiTables);

    /// @brief Initializes other processors present in the system.
    ///
    /// @param memory System memory information.
    /// @param bsp The APIC for the BSP.
    /// @param acpiTables The ACPI tables.
    /// @param ist The ISR table.
    void InitSmp(km::SystemMemory& memory, km::IApic *bsp, const acpi::AcpiTables& acpiTables, std::atomic_flag *launchScheduler, std::atomic<uint32_t>* remaining);
}
