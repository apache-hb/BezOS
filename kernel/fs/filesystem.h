#pragma once

#include "stream.h"

namespace bezos::fs
{
    void init();
    void mount(stream input);
}