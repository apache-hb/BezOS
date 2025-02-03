#pragma once

#include "drivers/fs/driver.hpp"

namespace km {
    struct TarPosixHeader {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char checksum[8];
        char typeflag;
        char linkname[100];
        char magic[6];
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];

        size_t getSize() const {
            size_t result = 0;
            for (size_t i = 0; i < 12; i++) {
                result = result * 8 + (size[i] - '0');
            }

            return result;
        }
    };

    static_assert(sizeof(TarPosixHeader) == 500);

    class TarFsDriver : public IFsDriver {
    public:
        TarFsDriver(IBlockDevice *device)
            : IFsDriver(device)
        { }
    };
}
