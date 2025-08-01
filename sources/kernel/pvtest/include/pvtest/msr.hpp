#pragma once

#include <stdint.h>

namespace pv {
    class IModelRegisterSet {
    public:
        virtual ~IModelRegisterSet() = default;

        /// @brief Get the value of a model specific register
        /// @param msr The MSR to get
        /// @return The value of the MSR
        virtual uint64_t rdmsr(uint32_t msr) = 0;

        /// @brief Set the value of a model specific register
        /// @param msr The MSR to set
        /// @param value The value to set the MSR to
        virtual void wrmsr(uint32_t msr, uint64_t value) = 0;
    };
}
