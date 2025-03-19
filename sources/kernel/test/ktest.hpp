#pragma once

#include "absl/container/flat_hash_map.h"

#include "arch/intrin.hpp"
#include "memory/memory.hpp"

namespace kmtest {
    struct PageKey {
        void *begin;

        constexpr bool operator==(const PageKey &other) const {
            return (uintptr_t(begin) & ~0xFFF) == (uintptr_t(other.begin) & ~0xFFF);
        }
    };
}

template<>
struct std::hash<kmtest::PageKey> {
    size_t operator()(const kmtest::PageKey &key) const {
        return std::hash<uintptr_t>{}(uintptr_t(key.begin) & ~0xFFF);
    }
};


namespace kmtest {
    class alignas(0x1000) IMmio {
    public:
        virtual ~IMmio() = default;

        void protect(km::PageFlags access);

        /// @brief Invoked one instruction before a read to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void readBegin() { }

        /// @brief Invoked one instruction after a read to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void readEnd() { }

        /// @brief Invoked one instruction before a write to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void writeBegin() { }

        /// @brief Invoked one instruction after a write to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void writeEnd() { }
    };

    class Machine {
        absl::flat_hash_map<PageKey, IMmio*> mMmioRegions;

    public:
        virtual ~Machine() = default;

        template<std::derived_from<IMmio> T>
        void addMmioRegion(T *mmio) {
            static_assert((alignof(T) % 0x1000) == 0, "MMIO region types must be page aligned");

            void *begin = mmio;
            void *end = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(mmio) + sizeof(T));

            for (void *ptr = begin; ptr < end; ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + 0x1000)) {
                PageKey key{ptr};
                mMmioRegions[key] = mmio;
            }
        }

        void reset(kmtest::Machine *machine);
        Machine *instance();
    };
}
