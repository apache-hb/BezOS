#pragma once

#include "crt.hpp"

#define MOODYCAMEL_MALLOC ::malloc
#define MOODYCAMEL_FREE ::free

#include <concurrentqueue.h>
