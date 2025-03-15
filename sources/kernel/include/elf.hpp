#pragma once

#include "fs2/interface.hpp"
#include "memory.hpp"
#include "process/process.hpp"

#include <bit>

#include <cstddef>
#include <cstdint>

#include <cstring>

namespace elf {
    struct Elf64Rela {
        uint64_t offset;
        uint64_t info;
        int64_t addend;
    };

    struct Elf64Dyn {
        uint64_t tag;
        uint64_t value;
    };

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

    enum class ElfOsAbi {
        eNone = 0,
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

        std::endian endian() const {
            return ident[kDataIdent] == 1 ? std::endian::little : std::endian::big;
        }

        Class elfClass() const {
            return Class(ident[kClassIdent]);
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

    static_assert(sizeof(Header) == 64);
    static_assert(sizeof(ProgramHeader) == 56);
    static_assert(sizeof(SectionHeader) == 64);
}

namespace km {
    namespace detail {
        OsStatus LoadMemorySize(std::span<const elf::ProgramHeader> phs, VirtualRange *range);
    }

    struct Program {
        /// @brief TLS init data if present.
        /// Pointer into process address space, marked as read-only.
        AddressMapping tls;

        /// @brief Process address space that has been allocated to load the program.
        AddressMapping loadMapping;

        /// @brief Entry point for the program.
        /// Pointer into process address space.
        const void *entry;
    };

    OsStatus LoadElfProgram(vfs2::IFileHandle *file, SystemMemory *memory, Process *process, Program *result);

    OsStatus LoadElf(std::unique_ptr<vfs2::IFileHandle> file, SystemMemory& memory, SystemObjects& objects, ProcessLaunch *result);
}
