#include "system/system.hpp"

void sys2::System::addObject(sm::RcuWeakPtr<IObject> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.insert(object);
}
