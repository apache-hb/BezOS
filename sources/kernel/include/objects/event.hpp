#pragma once

#if __STDC_HOSTED__
#   include "objects/hosted/event.hpp"
#else
#   include "objects/kernel/event.hpp"
#endif
