#pragma once

#include "isr/isr.hpp"

namespace km {
    class RuntimeIsrManager : public ILocalIsrManager {
    public:
        LocalIsrTable *getLocalIsrTable() override;
        static RuntimeIsrManager *instance();

        static void cpuInit();
    };
}
