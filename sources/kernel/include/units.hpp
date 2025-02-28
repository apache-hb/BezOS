#pragma once

#include "arch/paging.hpp"

// workaround for clang bug, nodiscard in a requires clause triggers a warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"

#include <mp-units/systems/si.h>
#include <mp-units/systems/iec.h>

#pragma clang diagnostic pop

namespace mp = mp_units;
namespace si = mp_units::si;
namespace iec = mp_units::iec;

namespace km::units {
    inline constexpr auto byte = iec::byte;
    inline constexpr struct page final : mp::named_unit<"page", mp::mag<x64::kPageSize> * byte> {} page;
    inline constexpr struct large_page final : mp::named_unit<"large_page", mp::mag<x64::kLargePageSize> * byte> {} large_page;
    inline constexpr struct huge_page final : mp::named_unit<"huge_page", mp::mag<x64::kHugePageSize> * byte> {} huge_page;
}
