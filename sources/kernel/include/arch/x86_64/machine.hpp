#pragma once

#include "arch/generic/machine.hpp"
#include "arch/x86_64/mmu.hpp"

#include "std/static_string.hpp"

#include "util/cpuid.hpp"

namespace arch {
    class MachineX86_64 : GenericMachine {
        sm::CpuId mLeaf0;
        sm::CpuId mLeaf1;
        sm::CpuId mLeaf7;

        sm::CpuId mLeafExt0;
        MmuX86_64 mMmu;

        char mVendor[sm::kVendorStringSize];
        char mBrand[sm::kBrandStringSize];
        char mHypervisor[sm::kVendorStringSize];

        static MmuX86_64 CreateMmu() noexcept {
            auto leaf = sm::CpuId::of(0x80000008);
            uintptr_t paddr = (leaf.eax >> 0) & 0xFF;
            uintptr_t vaddr = (leaf.eax >> 8) & 0xFF;

            return MmuX86_64(vaddr, paddr);
        }

        static MmuX86_64 DefaultMmu() noexcept {
            return MmuX86_64(48, 48);
        }

        MachineX86_64() noexcept
            : mLeaf0(sm::CpuId::of(0x0))
            , mLeaf1(sm::CpuId::of(0x1))
            , mLeaf7(sm::CpuId::of(0x7))
            , mLeafExt0(sm::CpuId::of(0x80000000))
            , mMmu(hasExtendedLeaf(0x80000008) ? CreateMmu() : DefaultMmu())
        {
            if (hasExtendedLeaf(0x80000004)) {
                sm::GetBrandString(mBrand);
            } else {
                strncpy(mBrand, "UNKNOWN", sizeof(mBrand));
            }

            sm::GetVendorString(mVendor);

            if (isHypervisorPresent()) {
                sm::GetHypervisorString(mHypervisor);
            } else {
                strncpy(mHypervisor, "NATIVE", sizeof(mHypervisor));
            }
        }

    public:
        using BrandString = stdx::StaticString<sm::kBrandStringSize>;
        using VendorString = stdx::StaticString<sm::kVendorStringSize>;

        static constexpr VendorString kIntelVendor = "GenuineIntel";
        static constexpr VendorString kAmdVendor = "AuthenticAMD";
        static constexpr VendorString kKvmVendor = "KVMKVMKVM\0\0\0";
        static constexpr VendorString kQemuTcgVendor = "TCGTCGTCGTCG";
        static constexpr VendorString kMicrosoftHvVendor = "Microsoft Hv";

        // GenericMachine - begin

        static MachineX86_64 GetInfo() noexcept {
            return MachineX86_64();
        }

        constexpr VendorString getVendorString() const noexcept {
            return mVendor;
        }

        constexpr BrandString getBrandString() const noexcept {
            return mBrand;
        }

        constexpr BrandString getCpuBrandString() const noexcept {
            return getBrandString();
        }

        constexpr BrandString getFpuBrandString() const noexcept {
            return "Integrated FPU";
        }

        constexpr bool isHypervisorPresent() const noexcept {
            static constexpr uint32_t kHypervisorBit = (1 << 31);
            return mLeaf1.ecx & kHypervisorBit;
        }

        constexpr VendorString getHypervisorName() const noexcept {
            return mHypervisor;
        }

        constexpr pci::VendorId getVendorId() const noexcept {
            auto brand = getBrandString();
            if (brand == kIntelVendor) {
                return pci::VendorId::eIntel;
            } else if (brand == kAmdVendor) {
                return pci::VendorId::eAMD;
            } else {
                return pci::VendorId::eInvalid;
            }
        }

        constexpr MmuX86_64 getMmu() const noexcept {
            return mMmu;
        }

        // GenericMachine - end

        // MachineX86_64 - begin

        constexpr uint32_t getMaxLeaf() const noexcept {
            return mLeaf0.eax;
        }

        constexpr bool hasLeaf(uint32_t leaf) const noexcept {
            return getMaxLeaf() >= leaf;
        }

        constexpr uint32_t getMaxExtendedLeaf() const noexcept {
            return mLeafExt0.eax;
        }

        constexpr bool hasExtendedLeaf(uint32_t leaf) const noexcept {
            return getMaxExtendedLeaf() >= leaf;
        }

        // MachineX86_64 - end
    };

    using Machine = MachineX86_64;
}
