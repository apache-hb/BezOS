#pragma once

#include <utility>

#include "util/absl.hpp"

namespace sys2::detail {
    template<typename T>
    class RangeTable {
    public:
        using Segment = T;
        using Range = decltype(std::declval<Segment>().range());
        using Address = typename Range::ValueType;
        using Map = sm::BTreeMap<Address, Segment>;
    private:
        Map mSegments;
    public:
        using Iterator = typename Map::iterator;

        constexpr RangeTable() noexcept = default;

        Map& segments() noexcept {
            return mSegments;
        }

        Iterator at(Address address) noexcept {
            return mSegments.find(address);
        }

        /// @brief Return an iterator to the segment that contains the given address.
        Iterator find(Address address) noexcept {
            if (auto it = mSegments.lower_bound(address); it != mSegments.end()) {
                auto& [_, segment] = *it;
                auto range = segment.range();
                if (range.contains(address)) {
                    return it;
                }
            }

            return mSegments.end();
        }

        std::pair<Iterator, Iterator> find(Range range) noexcept {
            auto begin = find(range.front);
            auto end = find(range.back);
            return { begin, end };
        }

        Iterator begin() noexcept {
            return mSegments.begin();
        }

        Iterator end() noexcept {
            return mSegments.end();
        }

        void insert(T&& segment) noexcept {
            auto range = segment.range();
            mSegments.insert({ range.back, std::move(segment) });
        }

        void erase(Iterator it) noexcept {
            mSegments.erase(it);
        }

        void erase(Address address) noexcept {
            mSegments.erase(address);
        }

        size_t segments() const noexcept {
            return mSegments.size();
        }
    };
}
