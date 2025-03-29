#pragma once

#include "system/handle.hpp"

namespace sys2 {
    class Mutex final : public IObject {
    public:
        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Mutex"; }
    };
}
