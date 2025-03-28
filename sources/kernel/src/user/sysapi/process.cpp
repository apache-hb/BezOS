#include "user/sysapi.hpp"

#include "gdt.hpp"
#include "log.hpp"
#include "elf.hpp"

#include "process/thread.hpp"
#include "process/system.hpp"
#include "process/schedule.hpp"

#include "fs2/vfs.hpp"
#include "fs2/utils.hpp"

#include "util/defer.hpp"

#include "syscall.hpp"

static char toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }

    return c;
}

static OsStatus CreateThread(km::Process *process, km::SystemMemory& memory, km::SystemObjects& objects, uintptr_t entry, stdx::String name, km::Thread **result) {
    km::IsrContext regs {
        .rip = entry,
        .cs = (km::SystemGdt::eLongModeUserCode * 0x8) | 0b11,
        .rflags = 0x202,
        .ss = (km::SystemGdt::eLongModeUserData * 0x8) | 0b11,
    };

    KmDebugMessage("[ELF] Entry point: ", km::Hex(regs.rip), "\n");

    //
    // Now allocate the stack for the main thread.
    //

    static constexpr size_t kStackSize = 0x4000;
    km::PageFlags flags = km::PageFlags::eUser | km::PageFlags::eData;
    km::AddressMapping mapping{};
    if (OsStatus status = AllocateMemory(memory.pmmAllocator(), process->ptes.get(), 4, &mapping)) {
        KmDebugMessage("[ELF] Failed to allocate stack memory: ", status, "\n");
        return status;
    }

    KmDebugMessage("[ELF] Stack: ", mapping, "\n");

    char *stack = (char*)memory.map(mapping.physicalRange(), flags);
    memset(stack, 0x00, kStackSize);
    memory.unmap(stack, kStackSize);

    km::Thread *main = objects.createThread(stdx::String(name), process);
    main->userStack = mapping;

    regs.rbp = (uintptr_t)mapping.vaddr + kStackSize;
    regs.rsp = (uintptr_t)mapping.vaddr + kStackSize;

    main->state = regs;
    *result = main;
    return OsStatusSuccess;
}

OsCallResult um::ProcessCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    km::SystemMemory &memory = *system->memory;
    OsProcessCreateInfo createInfo{};
    vfs2::VfsPath path;
    std::unique_ptr<vfs2::IFileHandle> handle = nullptr;

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if (OsStatus status = ReadPath(context, createInfo.Executable, &path)) {
        return km::CallError(status);
    }

    if (OsStatus status = system->vfs->open(path, std::out_ptr(handle))) {
        return km::CallError(status);
    }

    vfs2::INode *node = vfs2::GetHandleNode(handle.get());
    vfs2::NodeInfo nInfo = node->info();

    km::MemoryRange pteMemory = memory.pmmAllocate(256);

    stdx::String name = stdx::String(nInfo.name);
    for (char& c : name) {
        c = toupper(c);
    }

    km::ProcessCreateInfo processCreateInfo {
        .parent = context->process(),
        .privilege = x64::Privilege::eUser,
    };

    km::Process *process = nullptr;
    if (OsStatus status = system->objects->createProcess(stdx::String(name), pteMemory, processCreateInfo, &process)) {
        return km::CallError(status);
    }

    if (createInfo.ArgsBegin != nullptr && createInfo.ArgsEnd != nullptr) {
        stdx::Vector2<std::byte> args;
        if (OsStatus status = context->readArray((uintptr_t)createInfo.ArgsBegin, (uintptr_t)createInfo.ArgsEnd, 0x1000, &args)) {
            return km::CallError(status);
        }

        km::AddressMapping argsMapping{};

        if (OsStatus status = process->map(memory, km::Pages(args.count()), km::PageFlags::eUser | km::PageFlags::eRead, km::MemoryType::eWriteBack, &argsMapping)) {
            return km::CallError(status);
        }


        void *window = memory.map(argsMapping.physicalRange(), km::PageFlags::eData);
        if (window == nullptr) {
            return km::CallError(OsStatusOutOfMemory);
        }

        defer {
            memory.unmap(window, argsMapping.size);
        };

        memcpy(window, args.data(), args.count());

        process->args = km::ProcessArgs {
            .mapping = argsMapping,
            .size = args.count(),
        };
    }

    km::Program program{};
    if (OsStatus status = km::LoadElfProgram(handle.get(), memory, process, &program)) {
        return km::CallError(status);
    }

    process->tlsInit = program.tlsInit;

    km::Thread *main = nullptr;
    if (OsStatus status = CreateThread(process, memory, *system->objects, program.entry, name + " MAIN", &main)) {
        return km::CallError(status);
    }

    system->scheduler->addWorkItem(main);

    return km::CallOk(process->publicId());
}

OsCallResult um::ProcessDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userProcess = regs->arg0;
    uint64_t userExitCode = regs->arg1;

    km::Process *process = nullptr;

    if (OsStatus status = SelectOwningProcess(system, context, userProcess, &process)) {
        return km::CallError(status);
    }

    system->objects->exitProcess(process, userExitCode);

    if (process == context->process()) {
        km::YieldCurrentThread();
    }

    return km::CallOk(0zu);
}

OsCallResult um::ProcessCurrent(km::System *, km::CallContext *context, km::SystemCallRegisterSet *) {
    km::Process *process = context->process();
    if (process == nullptr) {
        return km::CallError(OsStatusNotFound);
    }

    return km::CallOk(process->publicId());
}

OsCallResult um::ProcessStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userStat = regs->arg1;

    km::Process *process = nullptr;

    if (OsStatus status = SelectOwningProcess(system, context, userHandle, &process)) {
        return km::CallError(status);
    }

    OsProcessHandle ppid = process->parentId();

    OsProcessInfo stat {
        .Parent = ppid,
        .ArgsBegin = (const struct OsProcessParam *)process->args.mapping.vaddr,
        .ArgsEnd = (const struct OsProcessParam *)((char*)process->args.mapping.vaddr + process->args.size),
        .Status = process->state,
        .ExitCode = process->exitCode,
    };

    if (OsStatus status = context->writeObject(userStat, stat)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}
