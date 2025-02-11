#include "fs2/tarfs.hpp"
#include "log.hpp"

using namespace vfs2;

static constexpr size_t kTarBlockSize = 512;

OsStatus vfs2::detail::ConvertTarPath(const char *path, VfsPath *result) {
    std::array<char, kTarNameSize> buffer{};

    //
    // Save the loop counter as we will need it to construct
    // the final path.
    //
    size_t i = 0;
    for (; i < kTarNameSize; i++) {
        char c = path[i];
        //
        // Tar guarantees that the name is null terminated so it
        // is safe to break out of the loop when we encounter it.
        //
        if (c == '\0') {
            break;
        }

        //
        // Transform the tar path separator to the OS path separator.
        //
        if (c == '/') {
            buffer[i] = OS_PATH_SEPARATOR;
        } else {
            buffer[i] = c;
        }
    }

    //
    // Validate the path to make sure it conforms to our path rules.
    //
    VfsStringView view(buffer.data(), i);
    if (!VerifyPathText(view)) {
        return OsStatusInvalidPath;
    }

    *result = VfsPath(VfsString(view));
    return OsStatusSuccess;
}

OsStatus vfs2::ParseTar(km::BlockDevice *media, TarParseOptions options, BTreeMap<VfsPath, TarPosixHeader> *result) {
    TarPosixHeader header{};
    uint64_t offset = 0;
    const uint64_t mediaSize = media->size();

    //
    // Read the tar archive in 512 byte blocks.
    // Using the media size as an upper bound for the loop.
    //
    while (offset < mediaSize) {
        size_t read = media->read(offset, &header, sizeof(header));
        if (read != sizeof(header)) {
            //
            // If this happens, either the media is corrupt. Or there
            // is a bug in the tarfs driver. Either way it is not safe
            // to continue.
            //
            KmDebugMessage("[TARFS] Failed to read header at offset: ", km::Hex(offset).pad(16), "\n");
            KmDebugMessage("[TARFS] The reported media size is: ", km::Hex(mediaSize).pad(16), "\n");
            KmDebugMessage("[TARFS] This could indicate a faulty block device.\n");
            return OsStatusInvalidInput;
        }

        if (!options.ignoreChecksum) {
            //
            // If the checksum is invalid, we should assume we have overrun
            // the tar file. Better to be safe than sorry when parsing data.
            //
            uint64_t checksum = header.reportedChecksum();
            uint64_t actual = header.actualChecksum();
            if (checksum != actual) {
                KmDebugMessage("[TARFS] Invalid checksum for entry: ", header.name, "\n");
                return OsStatusInvalidInput;
            }
        }

        if (header.name[0] == '\0') {
            //
            // If the name is empty, we have reached the end of the archive.
            //
            break;
        }

        //
        // While the sizeof(TarPosixHeader) is 500 bytes, the tar format
        // operates on 512 byte blocks. We need to align ourselves to the
        // next block.
        //
        offset += kTarBlockSize;

        //
        // This routine is only concerned with parsing headers,
        // so we skip over the file data.
        //
        offset += sm::roundup(header.getSize(), kTarBlockSize);

        VfsNodeType type = header.getType();
        if (type != VfsNodeType::eFile && type != VfsNodeType::eFolder) {
            //
            // We only care about files and folders, so we skip over
            // any other types of entries.
            //
            continue;
        }

        VfsPath path;
        if (OsStatus status = detail::ConvertTarPath(header.name, &path)) {
            return status;
        }

        result->insert({ path, header });
    }

    return OsStatusSuccess;
}
