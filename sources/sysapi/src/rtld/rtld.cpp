#include "rtld/rtld.h"

#include "bezos/facility/fs.h"
#include "bezos/facility/vmem.h"
#include "rtld/arch/sparcv9/relocation.hpp"
#include "rtld/arch/x86_64/relocation.hpp"
#include "rtld/load/elf.hpp"
#include "rtld/arch/arch.hpp"

#include <bezos/facility/process.h>
#include <bezos/start.h>

#include <stdlib.h>

namespace elf = os::elf;

template<typename T>
static OsStatus DeviceRead(OsDeviceHandle device, OsSize offset, T *result) {
    OsSize size = 0;
    OsDeviceReadRequest read {
        .BufferFront = reinterpret_cast<char*>(result),
        .BufferBack = reinterpret_cast<char*>(result) + sizeof(T),
        .Offset = offset,
        .Timeout = OS_TIMEOUT_INFINITE,
    };

    if (OsStatus status = OsDeviceRead(device, read, &size)) {
        return status;
    }

    if (size != sizeof(T)) {
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

template<typename T>
static OsStatus DeviceRead(OsDeviceHandle device, OsSize offset, OsSize count, T *result) {
    OsSize size = 0;
    OsDeviceReadRequest read {
        .BufferFront = reinterpret_cast<char*>(result),
        .BufferBack = reinterpret_cast<char*>(result) + sizeof(T) * count,
        .Offset = offset,
        .Timeout = OS_TIMEOUT_INFINITE,
    };

    if (OsStatus status = OsDeviceRead(device, read, &size)) {
        return status;
    }

    if (size != sizeof(T) * count) {
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

static OsStatus ElfReadHeader(OsDeviceHandle device, elf::Type expected, elf::Header *result) {
    elf::Header header;
    if (OsStatus status = DeviceRead(device, 0, &header)) {
        return status;
    }

    if (!header.isValid()) {
        return OsStatusInvalidData;
    }

    if (header.version != elf::Version::eCurrent) {
        return OsStatusInvalidData;
    }

    bool valid = (header.type == expected)
              && (header.endian() == std::endian::native)
              && (header.elfClass() == elf::Class::eClass64)
              && (header.ehsize == sizeof(elf::Header))
              && (header.phentsize == sizeof(elf::ProgramHeader))
              && (header.shentsize == sizeof(elf::SectionHeader));

    if (!valid) {
        return OsStatusInvalidData;
    }

    *result = header;
    return OsStatusSuccess;
}

#if 0
[[maybe_unused]]
static os::IRelocator *GetRelocator(os::elf::Machine machine) {
    if (machine == os::elf::Machine::eAmd64) {
        static os::Amd64Relocator sInstance;
        return &sInstance;
    } else if (machine == os::elf::Machine::eSparcV9) {
        static os::SparcV9Relocator sInstance;
        return &sInstance;
    } else {
        return nullptr;
    }
}

OsStatus RtldStartProgram(const RtldStartInfo *StartInfo) {
    elf::Header header;
    UniquePtr<elf::ProgramHeader[]> phs { nullptr, &free };
    OsStatus status = OsStatusSuccess;

    if ((status = ElfReadHeader(StartInfo->Program, elf::Type::eExecutable, &header))) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

    phs = UniqueArray<elf::ProgramHeader>(header.phnum);
    if (phs == nullptr) {
        status = OsStatusOutOfMemory;
        goto error;
    }

    if ((status = DeviceRead(StartInfo->Program, header.phoff, header.phnum, phs.get()))) {
        goto error;
    }

error:
    return status;
}

OsStatus RtldSoOpen(const RtldSoLoadInfo *LoadInfo, RtldSo *OutObject) {
    elf::Header header;
    UniquePtr<elf::ProgramHeader[]> phs { nullptr, &free };
    OsStatus status = OsStatusSuccess;

    if ((status = ElfReadHeader(LoadInfo->Object, elf::Type::eShared, &header))) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

    phs = UniqueArray<elf::ProgramHeader>(header.phnum);
    if (phs == nullptr) {
        status = OsStatusOutOfMemory;
        goto error;
    }

error:
    return status;
}

OsStatus RtldSoClose(RtldSo *Object) {
    return OsStatusSuccess;
}

OsStatus RtldSoSymbol(RtldSo *Object, RtldSoName Name, void **OutAddress) {
    return OsStatusNotFound;
}
#endif

OsStatus RtldStartProgram(const RtldStartInfo *StartInfo) {
    elf::Header header;
    OsStatus status = OsStatusSuccess;

    if ((status = ElfReadHeader(StartInfo->Program, elf::Type::eExecutable, &header))) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

error:
    return status;
}

OsStatus RtldSoOpen(const RtldSoLoadInfo *LoadInfo, RtldSo *OutObject) {
    elf::Header header;
    OsStatus status = OsStatusSuccess;

    if ((status = ElfReadHeader(LoadInfo->Object, elf::Type::eShared, &header))) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

error:
    return status;
}

OsStatus RtldSoClose(RtldSo *Object) {
    return OsStatusSuccess;
}

OsStatus RtldSoSymbol(RtldSo *Object, RtldSoName Name, void **OutAddress) {
    return OsStatusNotFound;
}