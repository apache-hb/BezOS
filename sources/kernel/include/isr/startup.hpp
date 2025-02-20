#pragma once

#include "isr/isr.hpp"

namespace km {
    class StartupIsrManager : public ILocalIsrManager {
    public:
        LocalIsrTable *getLocalIsrTable() override;
        static StartupIsrManager *instance();
    };
}
