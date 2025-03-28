#pragma once

#include "process/object.hpp"

#include "elf.hpp"
#include "arch/isr.hpp"
#include "memory/tables.hpp"
#include "std/string.hpp"

#include "util/absl.hpp"

namespace km {
    class AddressSpace;
    struct Process;
    struct Thread;

    struct ProcessCreateInfo {
        Process *parent;
        x64::Privilege privilege;
        uintptr_t userArgsBegin;
        uintptr_t userArgsEnd;
    };

    struct ProcessArgs {
        AddressMapping mapping;
        size_t size;
    };

    struct Process : public KernelObject {
        Process() = default;

        Process *parent = nullptr;

        RangeAllocator<PhysicalAddress> pmm;
        std::unique_ptr<AddressSpace> ptes;
        sm::FlatHashMap<OsHandle, KernelObject*> handles;
        OsProcessStateFlags state = eOsProcessRunning;
        ProcessArgs args;
        int64_t exitCode = 0;
        TlsInit tlsInit;

        void init(ProcessId id, stdx::String name, SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea, ProcessCreateInfo createInfo);

        bool isComplete() const;

        OsProcessHandle parentId() const {
            return parent ? parent->publicId() : OS_HANDLE_INVALID;
        }

        void terminate(OsProcessStateFlags state, int64_t exitCode);

        void addHandle(KernelObject *object);
        KernelObject *findHandle(OsHandle id);
        OsStatus removeHandle(OsHandle id);

        OsStatus map(km::SystemMemory& memory, size_t pages, PageFlags flags, MemoryType type, AddressMapping *mapping);
        OsStatus createTls(km::SystemMemory& memory, Thread *thread);
    };

    struct ProcessLaunch {
        Process *process;
        Thread *main;
    };
}
