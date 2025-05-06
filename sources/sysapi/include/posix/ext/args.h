#ifndef BEZOS_POSIX_EXT_ARGS_H
#define BEZOS_POSIX_EXT_ARGS_H 1

#include <bezos/handle.h>

#include <detail/cxx.h>

BZP_API_BEGIN

OS_DEFINE_GUID(kPosixInitGuid, 0x91482170, 0x01a1, 0x11f0, 0x8fa3, 0x93bdf5cdf14b);

struct OsPosixInitArgs {
    OsDeviceObject StandardIn;
    OsDeviceObject StandardOut;
    OsDeviceObject StandardError;
};

BZP_API_END

#endif /* BEZOS_POSIX_EXT_ARGS_H */
