#pragma once

#include "logger/logger.hpp"

constinit inline km::Logger InitLog { "INIT" };
constinit inline km::Logger VfsLog { "VFS" };
constinit inline km::Logger CpuLog { "CPU" };
constinit inline km::Logger XSaveLog { "XSAVE" };
constinit inline km::Logger MemLog { "MEM" };
constinit inline km::Logger SysLog { "SYS" };
constinit inline km::Logger AcpiLog { "ACPI" };
constinit inline km::Logger IsrLog { "ISR" };
constinit inline km::Logger PciLog { "PCI" };
constinit inline km::Logger BiosLog { "SMBIOS" };
constinit inline km::Logger TestLog { "TEST" };
constinit inline km::Logger ClockLog { "CLOCK" };
constinit inline km::Logger Ps2Log { "PS/2" };
constinit inline km::Logger TaskLog { "TASK" };
constinit inline km::Logger UserLog { "USER" };
