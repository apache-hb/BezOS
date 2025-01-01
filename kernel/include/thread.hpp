#pragma once

#include "arch/msr.hpp"

namespace km {
    constexpr x64::ModelRegister<0xC0000100, x64::RegisterAccess::eReadWrite> kGsBase;
    constexpr x64::ModelRegister<0xC0000101, x64::RegisterAccess::eReadWrite> kFsBase;
    constexpr x64::ModelRegister<0xC0000102, x64::RegisterAccess::eReadWrite> kKernelGsBase;
}
