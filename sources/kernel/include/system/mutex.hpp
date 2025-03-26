#pragma once

#include "system/handle.hpp"

namespace sys2 {
    class Mutex final : public IObject {
    public:
        void setName(ObjectName name) override;
        ObjectName getName() const override;

        stdx::StringView getClassName() const override { return "Mutex"; }

        OsStatus open(OsHandle *handle) override;
        OsStatus close(OsHandle handle) override;
    };
}
