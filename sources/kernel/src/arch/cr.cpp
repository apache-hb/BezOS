#include "arch/cr0.hpp"
#include "arch/cr4.hpp"
#include "arch/xcr0.hpp"

using Cr0Format = km::Format<x64::Cr0>;
using Cr4Format = km::Format<x64::Cr4>;
using Xcr0Format = km::Format<x64::Xcr0>;

using namespace stdx::literals;

template<size_t N>
static void formatFlag(stdx::StaticString<N>& result, bool set, stdx::StringView name) {
    if (set) {
        result.add(name);
        result.add(" "_sv);
    }
}

#define FORMAT_FLAG0(result, v, flag) formatFlag(result, v.test(x64::Cr0::flag), stdx::StringView(#flag))

Cr0Format::String Cr0Format::toString(x64::Cr0 value) {
    Cr0Format::String result;
    result.add(km::format(Hex(value.value()).pad(8, '0')));

    result.add(" [ "_sv);
    FORMAT_FLAG0(result, value, PG);
    FORMAT_FLAG0(result, value, CD);
    FORMAT_FLAG0(result, value, NW);
    FORMAT_FLAG0(result, value, AM);
    FORMAT_FLAG0(result, value, WP);
    FORMAT_FLAG0(result, value, NE);
    FORMAT_FLAG0(result, value, ET);
    FORMAT_FLAG0(result, value, TS);
    FORMAT_FLAG0(result, value, EM);
    FORMAT_FLAG0(result, value, MP);
    FORMAT_FLAG0(result, value, PE);
    result.add("]"_sv);

    return result;
}

#define FORMAT_FLAG4(result, v, flag) formatFlag(result, v.test(x64::Cr4::flag), stdx::StringView(#flag))

Cr4Format::String Cr4Format::toString(x64::Cr4 value) {
    Cr4Format::String result;
    result.add(km::format(Hex(value.value()).pad(8, '0')));

    result.add(" [ "_sv);
    FORMAT_FLAG4(result, value, VME);
    FORMAT_FLAG4(result, value, PVI);
    FORMAT_FLAG4(result, value, TSD);
    FORMAT_FLAG4(result, value, DE);
    FORMAT_FLAG4(result, value, PSE);
    FORMAT_FLAG4(result, value, PAE);
    FORMAT_FLAG4(result, value, MCE);
    FORMAT_FLAG4(result, value, PGE);
    FORMAT_FLAG4(result, value, PCE);
    FORMAT_FLAG4(result, value, OSFXSR);
    FORMAT_FLAG4(result, value, OSXMMEXCPT);
    FORMAT_FLAG4(result, value, UMIP);
    FORMAT_FLAG4(result, value, LA57);
    FORMAT_FLAG4(result, value, VMXE);
    FORMAT_FLAG4(result, value, SMXE);
    FORMAT_FLAG4(result, value, FSGSBASE);
    FORMAT_FLAG4(result, value, PCIDE);
    FORMAT_FLAG4(result, value, OSXSAVE);
    FORMAT_FLAG4(result, value, KL);
    FORMAT_FLAG4(result, value, SMEP);
    FORMAT_FLAG4(result, value, SMAP);
    FORMAT_FLAG4(result, value, PKE);
    result.add("]"_sv);

    return result;
}

void Xcr0Format::format(km::IOutStream& out, x64::Xcr0 value) {
    out.format(Hex(value.value()).pad(16));
    out.format(" [ "_sv);

    if (value.test(x64::Xcr0::FPU)) out.format("FPU "_sv);
    if (value.test(x64::Xcr0::SSE)) out.format("SSE "_sv);
    if (value.test(x64::Xcr0::AVX)) out.format("AVX "_sv);
    if (value.test(x64::Xcr0::BNDREG)) out.format("BNDREG "_sv);
    if (value.test(x64::Xcr0::BNDCSR)) out.format("BNDCSR "_sv);
    if (value.test(x64::Xcr0::OPMASK)) out.format("OPMASK "_sv);
    if (value.test(x64::Xcr0::ZMM_HI256)) out.format("ZMM_HI256 "_sv);
    if (value.test(x64::Xcr0::HI16_ZMM)) out.format("HI16_ZMM "_sv);
    if (value.test(x64::Xcr0::PKRU)) out.format("PKRU "_sv);
    if (value.test(x64::Xcr0::TILECONFIG)) out.format("TILECONFIG "_sv);
    if (value.test(x64::Xcr0::TILEDATA)) out.format("TILEDATA "_sv);

    out.format("]"_sv);
}
