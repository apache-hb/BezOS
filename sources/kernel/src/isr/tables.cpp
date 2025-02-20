#include "isr/startup.hpp"
#include "isr/runtime.hpp"
#include "thread.hpp"

static constinit km::LocalIsrTable gStartupIsrTable{};

CPU_LOCAL
static constinit km::CpuLocal<km::LocalIsrTable> tlsIsrTable{};

static constinit km::StartupIsrManager gStartupIsrManager{};
static constinit km::RuntimeIsrManager gRuntimeIsrManager{};

km::LocalIsrTable *km::StartupIsrManager::getLocalIsrTable() {
    return &gStartupIsrTable;
}

km::StartupIsrManager *km::StartupIsrManager::instance() {
    return &gStartupIsrManager;
}

km::LocalIsrTable *km::RuntimeIsrManager::getLocalIsrTable() {
    return &tlsIsrTable;
}

km::RuntimeIsrManager *km::RuntimeIsrManager::instance() {
    return &gRuntimeIsrManager;
}
