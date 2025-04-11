#include "arch/sparcv9/machine.hpp"

#include <frozen/map.hpp>

using namespace arch::sparcv9::detail;

static constexpr inline auto kMachines = frozen::make_map<uint8_t, SparcV9MachineInfo>({
    {
        eChipUltra1,
        SparcV9MachineInfo {
            .vendor = "SUN Microsystems",
            .cpuBrand = "UltraSPARC-T1 (Niagara)",
            .fpuBrand = "UltraSPARC-T1 integrated FPU",
            .pmuBrand = "Niagara-T1"
        }
    }
});

arch::MachineSparcV9 arch::MachineSparcV9::GetInfo() noexcept {

}
