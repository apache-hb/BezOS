#include "process/thread.hpp"

#include "process/schedule.hpp"
#include "xsave.hpp"

using namespace km;

void Thread::init(ThreadId id, stdx::String name, Process *process, AddressMapping kernelStack) {
    initHeader(std::to_underlying(id), eOsHandleThread, std::move(name));
    this->process = process;
    this->kernelStack = kernelStack;

    size_t xsaveSize = XSaveSize();
    if (xsaveSize != 0) {
        void *memory = aligned_alloc(alignof(x64::XSave), xsaveSize);
        memset(memory, 0, xsaveSize);
        xsave.reset((x64::XSave*)memory);
    }
}

OsStatus Thread::waitOnHandle(BaseObject *object, OsInstant _) {
    // TODO: timeout
    if (object->handleType() == eOsHandleProcess) {
        Process *process = static_cast<Process*>(object);
        while (!process->isComplete()) {
            YieldCurrentThread();
        }

        return OsStatusSuccess;
    } else if (object->handleType() == eOsHandleThread) {
        Thread *thread = static_cast<Thread*>(object);
        while (!thread->isComplete()) {
            YieldCurrentThread();
        }

        return OsStatusSuccess;
    } else {
        return OsStatusInvalidHandle;
    }
}
