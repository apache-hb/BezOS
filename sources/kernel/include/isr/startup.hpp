#pragma once

#include "isr/isr.hpp"

namespace km {
    class StartupIsrManager : public ILocalIsrManager {
        LocalIsrTable *getLocalIsrTable() override;

    public:
        static StartupIsrManager *instance();
    };
}
