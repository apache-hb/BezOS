#include "user/sysapi.hpp"

#include "elf.hpp"
#include "process/system.hpp"
#include "process/schedule.hpp"
#include "fs2/vfs.hpp"
#include "syscall.hpp"

OsCallResult um::ProcessCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsProcessCreateInfo createInfo{};
    vfs2::VfsPath path;
    std::unique_ptr<vfs2::IFileHandle> node = nullptr;
    km::ProcessLaunch launch{};

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if (OsStatus status = UserReadPath(context, createInfo.Executable, &path)) {
        return km::CallError(status);
    }

    if (OsStatus status = system->vfs->open(path, std::out_ptr(node))) {
        return km::CallError(status);
    }

    if (OsStatus status = km::LoadElf(std::move(node), *system->memory, *system->objects, &launch)) {
        return km::CallError(status);
    }

    system->scheduler->addWorkItem(launch.main);

    return km::CallOk(launch.process->publicId());
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
        .Status = process->state,
        .ExitCode = process->exitCode,
    };

    if (OsStatus status = context->writeObject(userStat, stat)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}
