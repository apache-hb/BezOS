#include "gdt.hpp"

#include "arch/intrin.hpp"

void KmInitGdt(const x64::GdtEntry *gdt, uint64_t count, uint64_t codeSelector, uint64_t dataSelector) {
    GDTR gdtr = {
        .limit = uint16_t((count * sizeof(x64::GdtEntry)) - 1),
        .base = (uint64_t)gdt
    };

    __lgdt(gdtr);

    asm volatile (
        // long jump to reload the CS register with the new code segment
        "pushq %[code]\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:"
        : /* no outputs */
        : [code] "r"(codeSelector * sizeof(x64::GdtEntry))
        : "memory", "rax"
    );

    asm volatile (
        // zero out all segments aside from ss
        "mov $0, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        // load the data segment into ss
        "movw %[data], %%ax\n"
        "mov %%ax, %%ss\n"
        : /* no outputs */
        : [data] "r"((uint16_t)(dataSelector * sizeof(x64::GdtEntry))) /* inputs */
        : "memory", "rax" /* clobbers */
    );
}

using GdtFormat = km::StaticFormat<x64::GdtEntry>;
using GdtString = GdtFormat::String;

using namespace stdx::literals;

GdtString GdtFormat::toString(x64::GdtEntry value) {
    GdtString result;
    uint64_t v = value.value();
    result.add(km::format(Hex(v).pad(16, '0')));

    if (v == 0) {
        result.add(" [ NULL ]"_sv);
        return result;
    }

    result.add(" [ "_sv);
    uint32_t limit = value.limit();
    if (limit != 0) {
        result.add("LIMIT = "_sv);
        result.add(km::format(Hex(limit).pad(6, '0')));
        result.add(" "_sv);
    }

    uint32_t base = value.base();
    if (base != 0) {
        result.add("BASE = "_sv);
        result.add(km::format(Hex(base).pad(8, '0')));
        result.add(" "_sv);
    }

    x64::Flags flags = value.flags();
    if (flags != x64::Flags::eNone) {
        result.add("FLAGS = [ "_sv);

        if (bool(flags & x64::Flags::eLong)) {
            result.add("LONG "_sv);
        }

        if (bool(flags & x64::Flags::eSize)) {
            result.add("SIZE "_sv);
        }

        if (bool(flags & x64::Flags::eGranularity)) {
            result.add("GRANULARITY "_sv);
        }

        result.add("] "_sv);
    }

    x64::Access access = value.access();
    if (access != x64::Access::eNone) {
        result.add("ACCESS = [ "_sv);

        if (bool(access & x64::Access::eCodeOrDataSegment)) {
            result.add("CODE/DATA "_sv);
        }

        if (bool(access & x64::Access::eExecutable)) {
            result.add("EXECUTABLE "_sv);
        }

        if (bool(access & x64::Access::eReadWrite)) {
            result.add("READ/WRITE "_sv);
        }

        if (bool(access & x64::Access::ePresent)) {
            result.add("PRESENT "_sv);
        }

        if (bool(access & x64::Access::eAccessed)) {
            result.add("ACCESSED "_sv);
        }

        if (bool(access & x64::Access::eEscalateDirection)) {
            result.add("DIRECTION "_sv);
        }

        result.add("] "_sv);
    }

    result.add("]"_sv);

    return result;
}
