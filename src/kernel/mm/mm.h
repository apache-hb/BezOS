#pragma once

#include "boot.h"

namespace mm
{
    void init(const array<E820Entry>& memory);
}