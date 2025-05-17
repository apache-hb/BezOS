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
constinit inline km::Logger SelfTestLog { "TEST" };
