#include "rtld/load/elf.hpp"

#include "rtld/rtld.h"

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
            //   && (header.endian() == std::endian::native)
              && (header.elfClass() == elf::Class::eClass64)
              && (header.machine == elf::Machine::eAmd64)
              && (header.ehsize == sizeof(elf::Header))
              && (header.phentsize == sizeof(elf::ProgramHeader))
              && (header.shentsize == sizeof(elf::SectionHeader));

    if (!valid) {
        return OsStatusInvalidData;
    }

    *result = header;
    return OsStatusSuccess;
}

OsStatus RtldStartProgram(const RtldStartInfo *StartInfo) {
    elf::Header header;
    elf::ProgramHeader *phs = nullptr;

    if (OsStatus status = ElfReadHeader(StartInfo->Program, elf::Type::eExecutable, &header)) {
        return status;
    }

    phs = (header.phnum) ? (elf::ProgramHeader*)malloc(sizeof(elf::ProgramHeader) * header.phnum) : nullptr;
    if (phs == nullptr) {
        goto outOfMemory;
    }

outOfMemory:
    if (phs) free(phs);
    return OsStatusOutOfMemory;
}
