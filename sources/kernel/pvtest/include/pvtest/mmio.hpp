#pragma once

#include "arch/paging.hpp"

#include "common/virtual_address.hpp"

namespace pv {
    class IMmio {
    public:
        virtual ~IMmio() = default;

        /// @brief Invoked one instruction before a read to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void readBegin(sm::VirtualAddress) { }

        /// @brief Invoked one instruction after a read to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void readEnd(sm::VirtualAddress) { }

        /// @brief Invoked one instruction before a write to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void writeBegin(sm::VirtualAddress) { }

        /// @brief Invoked one instruction after a write to this MMIO regions memory.
        /// @warning Invoked from a signal handling context.
        virtual void writeEnd(sm::VirtualAddress) { }

        virtual sm::VirtualAddressOf<x64::page> getMmioRegion() = 0;
    };
}
