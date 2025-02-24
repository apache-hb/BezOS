#pragma once

#include "apic.hpp"
#include "isr/isr.hpp"
#include "memory.hpp"

namespace km {
    /// @brief The stack size for each AP.
    constexpr size_t kStartupStackSize = x64::kPageSize * 8;

    /// @brief The amount of memory required to startup a single AP.
    constexpr size_t kStartupMemorySize = kStartupStackSize;

    size_t GetStartupMemorySize(const acpi::AcpiTables& acpiTables);

    using SmpInitCallback = void(*)(LocalIsrTable*, IApic*, void*);

    /// @brief Initializes other processors present in the system.
    ///
    /// @param memory System memory information.
    /// @param bsp The APIC for the BSP.
    /// @param acpiTables The ACPI tables.
    /// @param ist The ISR table.
    void InitSmp(
        km::SystemMemory& memory,
        km::IApic *bsp,
        const acpi::AcpiTables& acpiTables,
        SmpInitCallback callback,
        void *user
    );

    template<typename F>
    void InitSmp(SystemMemory& memory, IApic *bsp, const acpi::AcpiTables& acpiTables, F&& callback) {
        void *user = new F(std::forward<F>(callback));
        SmpInitCallback cb = [](LocalIsrTable *ist, IApic *apic, void *user) {
            auto *callback = reinterpret_cast<F*>(user);
            (*callback)(ist, apic);
        };

        InitSmp(memory, bsp, acpiTables, cb, user);
    }
}
