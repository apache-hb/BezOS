#pragma once

#include "isr/isr.hpp"

namespace km {
    class RuntimeIsrManager : public ILocalIsrManager {
        LocalIsrTable *getLocalIsrTable() override;

    public:
        static RuntimeIsrManager *instance();
    };
}
