#include "system2/process.hpp"
#include "system2/system.hpp"

[[nodiscard]]
OsStatus obj::Process::create(System *system, const ProcessCreateInfo& createInfo, sm::RcuSharedPtr<Process> *result [[outparam]]) noexcept [[clang::allocating]] {
    obj::IPhysicalMemoryService *pmmService = system->getPhysicalMemoryService();
    obj::IVirtualMemoryService *vmmService = system->getVirtualMemoryService();

    auto process = sm::rcuMakeShared<Process>(&system->getDomain());
    if (!process.isValid()) {
        return OsStatusOutOfMemory;
    }

    process->mSystem = system;
    process->setName(createInfo.getName());
    process->mParentProcess = createInfo.getParentProcess();

    if (OsStatus status = pmmService->createClient(process, std::out_ptr(process->mPhysicalMemoryClient))) {
        return status;
    }

    if (OsStatus status = vmmService->createClient(process, std::out_ptr(process->mVirtualMemoryClient))) {
        return status;
    }

    *result = process;
    return OsStatusSuccess;
}
