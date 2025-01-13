#include "drivers/driver.hpp"

extern "C" const km::DeviceDriver __drivers_start[];
extern "C" const km::DeviceDriver __drivers_end[];

std::span<const km::DeviceDriver> GetDeviceDrivers() {
    return std::span(__drivers_start, __drivers_end);
}
