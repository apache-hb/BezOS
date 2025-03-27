#include "system/process.hpp"
#include "system/system.hpp"

OsStatus sys2::CreateProcess(System *system, const ProcessCreateInfo& info, sm::RcuSharedPtr<Process> *process) {
    if (auto result = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), info)) {
        system->addObject(result.weak());
        *process = result;
        return OsStatusSuccess;
    }

    return OsStatusOutOfMemory;
}
