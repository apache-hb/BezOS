#pragma once

#include <utility>

#include "util/absl.hpp"

namespace sys2::detail {
    template<typename T, typename Super>
    class RangeTable {
        using CommandList = typename Super::CommandList;
        using Range = decltype(std::declval<T>().range());
        using Element = typename Range::Type;
        using Map = sm::BTreeMap<Element, T>;
    };
}
