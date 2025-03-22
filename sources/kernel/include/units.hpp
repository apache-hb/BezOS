#pragma once

#include "arch/paging.hpp"

#include "util/format.hpp"

// workaround for clang bug, nodiscard in a requires clause triggers a warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"

#include <mp-units/systems/si.h> // IWYU pragma: export
#include <mp-units/systems/iec.h> // IWYU pragma: export

#pragma clang diagnostic pop

namespace mp = mp_units;
namespace iec = mp_units::iec;

namespace si {
    using namespace mp_units::si;
    inline constexpr struct megahertz final : mp::named_unit<"MHz", mp::si::mega<si::hertz>> {} megahertz;
}

namespace km::units {
    inline constexpr auto byte = iec::byte;
    inline constexpr struct page final : mp::named_unit<"page", mp::mag<x64::kPageSize> * byte> {} page;
}

template<typename T>
struct km::Format<mp::quantity<si::hertz, T>> {
    static void format(km::IOutStream& out, mp::quantity<si::hertz, T> value) {
        static constexpr T kMhz = T(1'000'000);
        T count = T(value / si::hertz);
        if (count > kMhz) {
            out.format(count / kMhz, ".", count % kMhz, " MHz");
        } else {
            out.format(count, " Hz");
        }
    }
};
