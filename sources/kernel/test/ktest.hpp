#pragma once

#include "absl/container/flat_hash_map.h"

#include <sys/mman.h>
#include <sys/ucontext.h>

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
    /// @brief An mmio region
    /// @warning This object is mapped in shared memory and is accessed from a signal handling context.
    class alignas(0x1000) IMmio {
    public:
        virtual ~IMmio() = default;

        /// @brief Invoked one instruction before a read to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void readBegin(uintptr_t) { }

        /// @brief Invoked one instruction after a read to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void readEnd(uintptr_t) { }

        /// @brief Invoked one instruction before a write to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void writeBegin(uintptr_t) { }

        /// @brief Invoked one instruction after a write to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void writeEnd(uintptr_t) { }
    };

    struct MmioRegion {
        IMmio *mmio;
        size_t size;
    };

    class Machine {
        absl::flat_hash_map<PageKey, MmioRegion> mMmioRegions;

        static void installSigSegv();

        void protectMmioRegions();

        void sigsegv(mcontext_t *mcontext);

    public:
        virtual ~Machine();

        template<std::derived_from<IMmio> T, typename... Args>
        T *addMmioRegion(Args&&... args) {
            static_assert(alignof(T) % 0x1000 == 0, "MMIO regions must be page aligned");
            static_assert(sizeof(T) % 0x1000 == 0, "MMIO regions must be page aligned");

            void *backing = mmap(nullptr, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
            if (backing == MAP_FAILED) {
                perror("mmap");
                exit(1);
            }

            T *mmio = new (backing) T(std::forward<Args>(args)...);

            int err = mprotect(mmio, sizeof(T), PROT_NONE);
            if (err) {
                perror("mprotect");
                exit(1);
            }

            void *begin = mmio;
            void *end = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(mmio) + sizeof(T));

            MmioRegion region { mmio, sizeof(T) };

            for (void *ptr = begin; ptr < end; ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + 0x1000)) {
                PageKey key{ptr};
                mMmioRegions[key] = region;
            }

            return mmio;
        }

        template<typename T>
        static T *unprotect(T *mmio) {
            int err = mprotect(mmio, sizeof(T), PROT_READ | PROT_WRITE);
            if (err) {
                perror("mprotect");
                exit(1);
            }

            return mmio;
        }

        static void setup();
        static void finalize();

        void reset(kmtest::Machine *machine);
        Machine *instance();
    };
}
