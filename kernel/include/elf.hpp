#pragma once

#include "memory.hpp"
#include "process/process.hpp"

#include <bit>

#include <cstddef>
#include <cstdint>

#include <cstring>
#include <expected>
#include <span>

namespace elf {
    enum class ElfType : uint16_t {
        eNone = 0,
        eRelocatable = 1,
        eExecutable = 2,
        eShared = 3,
        eCore = 4,
    };

    enum class ElfMachine : uint16_t {
        eNone = 0,
        eSparc = 2,
        eSparcV9 = 43,
        eAmd64 = 62,
        eArm64 = 183,
    };

    enum class ElfVersion : uint32_t {
        eNone = 0,
        eCurrent = 1,
    };

    enum class ElfClass {
        eNone = 0,
        eClass32 = 1,
        eClass64 = 2,
    };

    enum class ElfOsAbi {
        eNone = 0,
    };

    struct ElfHeader {
        static constexpr size_t kClassIdent = 4;
        static constexpr size_t kDataIdent = 5;
        static constexpr size_t kObjVersionIdent = 6;
        static constexpr size_t kOsAbiIdent = 7;
        static constexpr size_t kOsAbiVersionIdent = 8;

        static constexpr uint8_t kMagic[4] = { 0x7F, 'E', 'L', 'F' };

        uint8_t ident[16];
        ElfType type;
        ElfMachine machine;
        ElfVersion version;
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

        std::endian endian() const {
            return ident[kDataIdent] == 1 ? std::endian::little : std::endian::big;
        }

        ElfClass elfClass() const {
            return ElfClass(ident[kClassIdent]);
        }

        ElfOsAbi osAbi() const {
            return ElfOsAbi(ident[kOsAbiIdent]);
        }

        uint8_t osAbiVersion() const {
            return ident[kOsAbiVersionIdent];
        }

        uint8_t objVersion() const {
            return ident[kObjVersionIdent];
        }

        bool isValid() const {
            return std::memcmp(ident, kMagic, sizeof(kMagic)) == 0;
        }
    };

    struct ElfProgramHeader {
        uint32_t type;
        uint32_t flags;
        uint64_t offset;
        uint64_t vaddr;
        uint64_t paddr;
        uint64_t filesz;
        uint64_t memsz;
        uint64_t align;
    };

    struct ElfSectionHeader {
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

    static_assert(sizeof(ElfHeader) == 64);
    static_assert(sizeof(ElfProgramHeader) == 56);
    static_assert(sizeof(ElfSectionHeader) == 64);
}

namespace km {
    OsStatus LoadElf(std::unique_ptr<vfs2::IVfsNodeHandle> file, SystemMemory& memory, SystemObjects& objects, ProcessLaunch *result);

#if 0
    std::expected<Process, bool> LoadElf(std::span<const uint8_t> program, uint32_t id, SystemMemory& memory);
#endif
}
