#pragma once

#include <bezos/status.h>
#include <bezos/facility/debug.h>

#include <posix/errno.h>

#include <source_location>

[[gnu::visibility("hidden")]]
void Unimplemented(int error = ENOSYS, std::source_location location = std::source_location::current());
