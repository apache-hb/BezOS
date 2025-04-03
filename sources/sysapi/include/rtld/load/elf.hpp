#pragma once

#include <stdint.h>
#include <stddef.h>

namespace os::elf {
    enum class Type : uint16_t {
        eNone = 0,
        eRelocatable = 1,
        eExecutable = 2,
        eShared = 3,
        eCore = 4,
    };

    enum class Machine : uint16_t {
        eNone = 0,
        eSparc = 2,
        eSparcV9 = 43,
        eAmd64 = 62,
        eArm64 = 183,
    };

    enum class Version : uint32_t {
        eNone = 0,
        eCurrent = 1,
    };

    enum class Class {
        eNone = 0,
        eClass32 = 1,
        eClass64 = 2,
    };

    enum class OsAbi {
        eNone = 0,
    };

    enum class ProgramHeaderType : uint32_t {
        eLoad = 0x1,
        eDynamic = 0x2,
        eTls = 0x7,
    };

    struct ProgramHeader {
        ProgramHeaderType type;
        uint32_t flags;
        uint64_t offset;
        uint64_t vaddr;
        uint64_t paddr;
        uint64_t filesz;
        uint64_t memsz;
        uint64_t align;
    };

    struct SectionHeader {
        uint32_t name;
        uint32_t type;
        uint64_t flags;
        uint64_t addr;
        uint64_t offset;
        uint64_t size;
        uint32_t link;
        uint32_t info;
        uint64_t addralign;
        uint64_t entsize;
    };

    struct Header {
        static constexpr size_t kClassIdent = 4;
        static constexpr size_t kDataIdent = 5;
        static constexpr size_t kObjVersionIdent = 6;
        static constexpr size_t kOsAbiIdent = 7;
        static constexpr size_t kOsAbiVersionIdent = 8;

        static constexpr uint8_t kMagic[4] = { 0x7F, 'E', 'L', 'F' };

        uint8_t ident[16];
        Type type;
        Machine machine;
        Version version;
        uint64_t entry;
        uint64_t phoff;
        uint64_t shoff;
        uint32_t flags;
        uint16_t ehsize;
        uint16_t phentsize;
        uint16_t phnum;
        uint16_t shentsize;
        uint16_t shnum;
        uint16_t shstrndx;

#if 0
        std::endian endian() const {
            return ident[kDataIdent] == 1 ? std::endian::little : std::endian::big;
        }
#endif

        Class elfClass() const {
            return Class(ident[kClassIdent]);
        }

        OsAbi osAbi() const {
            return OsAbi(ident[kOsAbiIdent]);
        }

        uint8_t osAbiVersion() const {
            return ident[kOsAbiVersionIdent];
        }

        uint8_t objVersion() const {
            return ident[kObjVersionIdent];
        }

        bool isValid() const;
    };

    static_assert(sizeof(Header) == 64);
    static_assert(sizeof(ProgramHeader) == 56);
    static_assert(sizeof(SectionHeader) == 64);
}
