#include "system2/thread.hpp"
#include "system2/system.hpp"

static constexpr size_t kMinimumPageTableCount = 16;

OsStatus obj::Thread::create(System *system, const ThreadCreateInfo& createInfo, sm::RcuSharedPtr<Thread> *result [[outparam]]) noexcept [[clang::allocating]] {
    auto thread = sm::rcuMakeShared<Thread>(&system->getDomain());
    if (!thread.isValid()) {
        return OsStatusOutOfMemory;
    }

    thread->mSystem = system;
    thread->setName(createInfo.getName());
    thread->mParentProcess = createInfo.getParentProcess();

    *result = thread;
    return OsStatusSuccess;
}
