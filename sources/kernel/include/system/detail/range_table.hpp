#pragma once

#include <utility>

#include "util/absl.hpp"

namespace sys::detail {
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
            auto begin = mSegments.upper_bound(range.front);
            auto end = mSegments.lower_bound(range.back);

            if (begin != mSegments.end()) {
                auto& [_, segment] = *begin;
                auto segRange = segment.range();
                if (segRange.contains(range)) {
                    return { begin, begin };
                }

                if (!range.intersects(segRange)) {
                    begin = mSegments.end();
                    end = mSegments.end();
                }
            }

            if (end == mSegments.end() && begin != mSegments.end()) {
                --end;
            }

            if (end != mSegments.end()) {
                auto& [_, segment] = *end;
                auto segRange = segment.range();
                if (segRange.front == range.back || !range.intersects(segRange)) {
                    --end;
                }
            }

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

        Iterator erase(Iterator it) noexcept {
            return mSegments.erase(it);
        }

        Iterator erase(Address address) noexcept {
            return mSegments.erase(address);
        }

        size_t segments() const noexcept {
            return mSegments.size();
        }
    };
}
