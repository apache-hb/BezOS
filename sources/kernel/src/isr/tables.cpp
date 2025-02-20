#include "isr/startup.hpp"
#include "isr/runtime.hpp"
#include "thread.hpp"

static constinit km::LocalIsrTable gStartupIsrTable{};

CPU_LOCAL
static constinit km::CpuLocal<km::LocalIsrTable> tlsIsrTable{};

static constinit km::StartupIsrManager gStartupIsrManager{};
static constinit km::RuntimeIsrManager gRuntimeIsrManager{};

static constinit km::ILocalIsrManager *gIsrManager = &gStartupIsrManager;

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

void km::RuntimeIsrManager::cpuInit() {
    std::fill(tlsIsrTable->begin(), tlsIsrTable->end(), km::DefaultIsrHandler);
}

km::LocalIsrTable *km::GetLocalIsrTable() {
    return gIsrManager->getLocalIsrTable();
}

void km::SetIsrManager(km::ILocalIsrManager *manager) {
    gIsrManager = manager;
}

void km::EnableCpuLocalIsrTable() {
    SetIsrManager(km::RuntimeIsrManager::instance());
}
