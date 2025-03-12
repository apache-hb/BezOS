#include "fs2/tarfs.hpp"
#include "bezos/status.h"
#include "fs2/node.hpp"
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
    // Folder names in tar archives are always terminated with a
    // '/' character. We need to remove this character from the
    // path.
    //
    if (i > 0 && buffer[i - 1] == OS_PATH_SEPARATOR) {
        i--;
    }

    //
    // tar paths can contain '.' to indicate the current directory.
    // We need to remove these segments from the path.
    //
    VfsStringView view(buffer.data(), i);

    while (view.startsWith(".\0")) {
        view = view.substr(2, view.count() - 2);
    }

    //
    // Validate the path to make sure it conforms to our path rules.
    //
    if (!VerifyPathText(view)) {
        return OsStatusInvalidPath;
    }

    *result = VfsPath(VfsString(view));
    return OsStatusSuccess;
}

OsStatus vfs2::ParseTar(km::BlockDevice *media, TarParseOptions options, sm::BTreeMap<VfsPath, TarEntry> *result) {
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

        if (header.name[0] == '\0') {
            //
            // If the name is empty, we have reached the end of the archive.
            //
            break;
        }

        if (header.magic != TarPosixHeader::kMagic) {
            //
            // If the magic is invalid, we may have overrun the tar file.
            //
            KmDebugMessage("[TARFS] Invalid magic for entry at offset: ", km::Hex(offset).pad(16), "\n");
            KmDebugMessage("[TARFS] Expected: '", TarPosixHeader::kMagic, "' Actual: '", header.magic, "'\n");
            break;
        }

        if (!options.ignoreChecksum) {
            //
            // If the checksum is invalid, we should assume we have overrun
            // the tar file. Better to be safe than sorry when parsing data.
            //
            uint64_t checksum = header.reportedChecksum();
            uint64_t actual = header.actualChecksum();
            if (checksum != actual) {
                KmDebugMessage("[TARFS] Invalid checksum for entry: '", header.name, "'\n");
                KmDebugMessage("[TARFS] Reported: ", km::Hex(checksum).pad(8), " Actual: ", km::Hex(actual).pad(8), "\n");
                return OsStatusInvalidInput;
            }
        }

        //
        // While the sizeof(TarPosixHeader) is 500 bytes, the tar format
        // operates on 512 byte blocks. We need to align ourselves to the
        // next block.
        //
        offset += kTarBlockSize;

        //
        // Save the offset of the data for this entry.
        //
        uint64_t dataOffset = offset;

        if (header.getSize() + offset > mediaSize) {
            //
            // If the size of the file is larger than the media size,
            // the tar file is corrupt.
            //
            KmDebugMessage("[TARFS] File size exceeds media size: ", header.name, "\n");
            return OsStatusInvalidInput;
        }

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
            KmDebugMessage("[TARFS] Failed to convert path: ", header.name, " = ", status, "\n");
            return status;
        }

        result->insert({ path, TarEntry(header, dataOffset) });
    }

    return OsStatusSuccess;
}

//
// tarfs file implementation
//

OsStatus TarFsNode::read(ReadRequest request, ReadResult *result) {
    if (request.offset >= mHeader.getSize()) {
        result->read = 0;
        return OsStatusEndOfFile;
    }

    uint64_t remaining = mHeader.getSize() - request.offset;
    uint64_t toRead = std::min(remaining, (uintptr_t)request.end - (uintptr_t)request.begin);

    km::BlockDevice *media = mMount->media();
    size_t read = media->read(mOffset + request.offset, request.begin, toRead);
    result->read = read;
    return OsStatusSuccess;
}

OsStatus TarFsNode::query(sm::uuid uuid, const void *, size_t, IHandle **handle) {
    if (uuid == kOsFileGuid) {
        auto *file = new (std::nothrow) TFileHandle<TarFsNode>(this);
        if (!file) {
            return OsStatusOutOfMemory;
        }

        *handle = file;
        return OsStatusSuccess;
    }

    if (uuid == kOsFolderGuid) {
        auto *folder = new (std::nothrow) TFolderHandle<TarFsNode>(this);
        if (!folder) {
            return OsStatusOutOfMemory;
        }

        *handle = folder;
        return OsStatusSuccess;
    }

    return OsStatusNotSupported;
}

NodeInfo TarFsNode::info() {
    return NodeInfo {
        .name = mHeader.name,
        .mount = mMount,
        .parent = mParent,
    };
}

OsStatus TarFsNode::stat(NodeStat *result) {
    size_t size = mHeader.getSize();

    *result = NodeStat {
        .logical = size,
        .blksize = kTarBlockSize,
        .blocks = sm::roundup(size, kTarBlockSize) / kTarBlockSize,
    };

    return OsStatusSuccess;
}

//
// tarfs mount implementation
//

OsStatus TarFsMount::walk(const VfsPath& path, INode **folder) {
    if (path.segmentCount() == 1) {
        *folder = mRootNode;
        return OsStatusSuccess;
    }

    INode *current = mRootNode;

    for (auto segment : path.parent()) {
        std::unique_ptr<IHandle> handle;
        if (OsStatus status = current->query(kOsFolderGuid, nullptr, 0, std::out_ptr(handle))) {
            return status;
        }

        IFolderHandle *folder = static_cast<IFolderHandle*>(handle.get());

        INode *child = nullptr;
        if (OsStatus status = folder->lookup(segment, &child)) {
            return status;
        }

        NodeInfo info = child->info();

        if (info.mount != this) {
            return OsStatusInvalidType;
        }

        current = child;
    }

    *folder = current;
    return OsStatusSuccess;
}

TarFsMount::TarFsMount(TarFs *tarfs, sm::SharedPtr<km::IBlockDriver> block)
    : IVfsMount(tarfs)
    , mBlock(block)
    , mMedia(mBlock.get())
    , mRootNode(new FolderNode(nullptr, this, VfsString("")))
{
    sm::BTreeMap<VfsPath, TarEntry> headers;
    if (OsStatus status = ParseTar(&mMedia, TarParseOptions{}, &headers)) {
        KmDebugMessage("[TARFS] Failed to parse tar archive: ", status, "\n");
        return;
    }

    for (auto& [path, header] : headers) {
        VfsNodeType type = header.header.getType();
        VfsStringView name = path.name();

        INode *parent = nullptr;
        if (OsStatus status = walk(path, &parent)) {
            KmDebugMessage("[TARFS] Failed to find parent for '", path, "' : ", status, "\n");
            continue;
        }

        std::unique_ptr<INode> node = nullptr;
        if (type == VfsNodeType::eFile) {
            node.reset(new (std::nothrow) TarFsNode(header, parent, this));
        } else if (type == VfsNodeType::eFolder) {
            node.reset(new (std::nothrow) TarFsFolder(header, parent, this));
        } else {
            KmDebugMessage("[TARFS] Invalid node type for '", path, "'\n");
            continue;
        }

        if (node == nullptr) {
            KmDebugMessage("[TARFS] Failed to create node for '", path, "'\n");
            continue;
        }

        std::unique_ptr<vfs2::IHandle> folder;
        if (OsStatus status = parent->query(kOsFolderGuid, nullptr, 0, std::out_ptr(folder))) {
            KmDebugMessage("[TARFS] Failed to query folder for '", path, "' : ", status, "\n");
            continue;
        }

        vfs2::IFolderHandle *handle = static_cast<vfs2::IFolderHandle*>(folder.get());
        if (OsStatus status = handle->mknode(name, node.release())) {
            KmDebugMessage("[TARFS] Failed to add folder '", path, "' : ", status, "\n");
        }
    }
}

OsStatus TarFsMount::root(INode **node) {
    *node = mRootNode;
    return OsStatusSuccess;
}

//
// tarfs driver implementation
//

OsStatus TarFs::mount(IVfsMount **) {
    // TODO: until I have a good solution for passing mount parameters
    // this will remain unimplemented.
    return OsStatusNotSupported;
}

OsStatus TarFs::unmount(IVfsMount *mount) {
    delete mount;
    return OsStatusSuccess;
}

OsStatus TarFs::createMount(IVfsMount **mount, sm::SharedPtr<km::IBlockDriver> block) {
    TarFsMount *result = new(std::nothrow) TarFsMount(this, block);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    *mount = result;
    return OsStatusSuccess;
}

TarFs& TarFs::instance() {
    static TarFs sDriver{};
    return sDriver;
}
