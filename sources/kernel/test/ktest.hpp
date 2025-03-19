#pragma once

#include "absl/container/flat_hash_map.h"
#include "memory/table_allocator.hpp"
#include "panic.hpp"

#include <gtest/gtest.h>
#include <sys/mman.h>
#include <sys/ucontext.h>

struct cs_insn;

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
        // pad out the vtable pointer
        [[maybe_unused]]
        char mPadding[0x1000 - 8];
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
        /// @brief The MMIO object handler
        IMmio *mmio;

        /// @brief The host address of the mmio state
        void *host;

        /// @brief Base address of this mmio region in the guest segment
        void *guest;
        size_t size;
    };

    class IMsrDevice {
    public:
        virtual ~IMsrDevice() = default;

        virtual uint64_t rdmsr(uint32_t) { KM_PANIC("unknown rdmsr"); }
        virtual void wrmsr(uint32_t, uint64_t) { KM_PANIC("unknown wrmsr"); }
    };

    class Machine {
        int mAddressSpaceFd;

        km::PageTableAllocator mHostAllocator;

        /// @brief Host address space
        void *mHostAddressSpace;

        absl::flat_hash_map<PageKey, MmioRegion> mMmioRegions;
        absl::flat_hash_map<uint32_t, IMsrDevice*> mMsrDevices;

        static void installSigSegv();

        void protectMmioRegions();

        void sigsegv(mcontext_t *mcontext);

        void rdmsr(mcontext_t *mcontext, cs_insn *insn);
        void wrmsr(mcontext_t *mcontext, cs_insn *insn);
        void mmio(mcontext_t *mcontext, cs_insn *insn);

        void initAddressSpace();

    public:
        virtual ~Machine();

        template<typename T>
        T *createMmioObject() {
            static_assert(alignof(T) % 0x1000 == 0, "MMIO regions must be page aligned");

            void *host = mHostAllocator.allocate(sm::roundup<size_t>(sizeof(T), 0x1000) / 0x1000);
            if (!host) {
                KM_PANIC("failed to allocate mmio region");
            }

            return new (host) T();
        }

        void addMmioRegion(IMmio *device, void *state, void *guest, size_t size) {
            MmioRegion region { device, state, guest, size };

            uintptr_t hostOffset = (uintptr_t)state - (uintptr_t)mHostAddressSpace;

            void *address = mmap(guest, size, PROT_NONE, MAP_FIXED | MAP_SHARED, mAddressSpaceFd, hostOffset);
            if (address == MAP_FAILED) {
                perror("mmap");
                exit(1);
            }

            ASSERT_EQ(address, guest) << "mmap failed to map at the requested address";

            for (void *ptr = guest; ptr < reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(guest) + size); ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + 0x1000)) {
                PageKey key{ptr};
                mMmioRegions[key] = region;
            }
        }

        void removeMmioRegion(void *guest, size_t size) {
            for (void *ptr = guest; ptr < reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(guest) + size); ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + 0x1000)) {
                PageKey key{ptr};
                mMmioRegions.erase(key);
            }

            int err = munmap(guest, size);
            if (err) {
                perror("munmap");
                exit(1);
            }
        }

        void moveMmioRegion(void *to, void *from, size_t size) {
            auto it = mMmioRegions.extract(PageKey{from});
            if (it.empty()) {
                throw std::runtime_error("mmio region not found");
            }

            auto &region = it.mapped();

            removeMmioRegion(from, size);
            addMmioRegion(region.mmio, region.host, to, size);
        }

        void msr(uint32_t msr, IMsrDevice *device) {
            mMsrDevices[msr] = device;
        }

        Machine();

        static void setup();
        static void finalize();

        void reset(kmtest::Machine *machine);
        Machine *instance();
    };
}
