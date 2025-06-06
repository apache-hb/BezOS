#include "gdt.hpp"

#include "arch/intrin.hpp"

void km::InitGdt(const void *gdt, size_t size, size_t codeSelector, size_t dataSelector) {
    GDTR gdtr = {
        .limit = uint16_t(size - 1),
        .base = (uint64_t)gdt,
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
        : [data] "r"(uint16_t(dataSelector * sizeof(x64::GdtEntry))) /* inputs */
        : "memory", "rax" /* clobbers */
    );
}

void km::InitGdt(std::span<const x64::GdtEntry> gdt, size_t codeSelector, size_t dataSelector) {
    InitGdt(gdt.data(), gdt.size_bytes(), codeSelector, dataSelector);
}

using GdtFormat = km::Format<x64::GdtEntry>;
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

    result.add("RING "_sv);
    uint8_t ring = uint8_t(access & x64::Access::eRing3) >> 5;
    result.add(km::format(ring));

    result.add(" ]"_sv);

    return result;
}

static constexpr km::SystemGdt kBootGdt = {
    .entries = {
        [km::SystemGdt::eNull] = x64::GdtEntry::null(),
        [km::SystemGdt::eRealModeCode] = x64::GdtEntry(x64::Flags::eRealMode, x64::Access::eCode, 0xffffffff),
        [km::SystemGdt::eRealModeData] = x64::GdtEntry(x64::Flags::eRealMode, x64::Access::eData, 0xffffffff),
        [km::SystemGdt::eProtectedModeCode] = x64::GdtEntry(x64::Flags::eProtectedMode, x64::Access::eCode, 0xffffffff),
        [km::SystemGdt::eProtectedModeData] = x64::GdtEntry(x64::Flags::eProtectedMode, x64::Access::eData, 0xffffffff),
        [km::SystemGdt::eLongModeCode] = x64::GdtEntry(x64::Flags::eLongMode, x64::Access::eCode, 0xffffffff),
        [km::SystemGdt::eLongModeData] = x64::GdtEntry(x64::Flags::eLongMode, x64::Access::eData, 0xffffffff),
        [km::SystemGdt::eLongModeUserData] = x64::GdtEntry(x64::Flags::eLongMode, x64::Access::eData | x64::Access::eRing3, 0xffffffff),
        [km::SystemGdt::eLongModeUserCode] = x64::GdtEntry(x64::Flags::eLongMode, x64::Access::eCode | x64::Access::eRing3, 0xffffffff),
    },
};

km::SystemGdt km::GetBootGdt() noexcept {
    return kBootGdt;
}

km::SystemGdt km::GetSystemGdt(const x64::TaskStateSegment *tss) {
    SystemGdt gdt = GetBootGdt();

    gdt.setTss(tss);

    return gdt;
}
