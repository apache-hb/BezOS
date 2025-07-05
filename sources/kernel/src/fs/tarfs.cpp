#include "fs/tarfs.hpp"
#include "bezos/status.h"
#include "fs/file.hpp"
#include "fs/identify.hpp"
#include "fs/iterator.hpp"
#include "fs/node.hpp"
#include "fs/query.hpp"
#include "fs/utils.hpp"
#include "log.hpp"
#include "logger/logger.hpp"

#include <ranges>

using namespace vfs;

static constexpr size_t kTarBlockSize = 512;

static constinit km::Logger TarLog { "TARFS" };

OsStatus vfs::detail::ConvertTarPath(const char *path, VfsPath *result) {
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

OsStatus vfs::ParseTar(km::BlockDevice *media, TarParseOptions options, sm::AbslBTreeMap<VfsPath, TarEntry> *result) {
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
            TarLog.errorf("Failed to read header at offset: ", km::Hex(offset).pad(16));
            TarLog.errorf("The reported media size is: ", km::Hex(mediaSize).pad(16));
            TarLog.errorf("This could indicate a faulty block device.");
            return OsStatusInvalidInput;
        }

        if (header.name[0] == '\0') {
            //
            // If the name is empty, we have probably reached the end of the archive.
            //
            break;
        }

        if (header.magic != TarPosixHeader::kMagic) {
            //
            // If the magic is invalid, we may have overrun the tar file.
            //
            TarLog.errorf("Invalid magic for entry at offset: ", km::Hex(offset).pad(16));
            TarLog.errorf("Expected: '", TarPosixHeader::kMagic, "' Actual: '", header.magic, "'");
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
                TarLog.errorf("Invalid checksum for entry: '", header.name, "'");
                TarLog.errorf("Reported: ", km::Hex(checksum).pad(8), " Actual: ", km::Hex(actual).pad(8));
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
            TarLog.errorf("File size exceeds media size: '", header.name, "'");
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

        if (strncmp(header.name, "./", 3) == 0) {
            //
            // Skip the current directory entry.
            //
            continue;
        }

        VfsPath path;
        if (OsStatus status = detail::ConvertTarPath(header.name, &path)) {
            TarLog.errorf("Failed to convert path: ", header.name, " = ", OsStatusId(status));
            return status;
        }

        result->insert({ path, TarEntry(header, dataOffset) });
    }

    return OsStatusSuccess;
}

//
// tarfs node implementation
//

void TarFsNode::init(sm::RcuWeakPtr<INode> parent, VfsString name, sys::NodeAccess) {
    mParent = parent;
    strncpy(mHeader.name, name.data(), sizeof(mHeader.name));
}

NodeInfo TarFsNode::info() {
    //
    // TarFs does not currently support writing so we always return RX.
    //
    return NodeInfo {
        .name = stdx::StringView(mHeader.name, mHeader.name + strnlen(mHeader.name, sizeof(mHeader.name))),
        .mount = mMount,
        .parent = mParent,
        .access = sys::NodeAccess::RX,
    };
}

//
// tarfs file implementation
//

static constexpr inline InterfaceList kFileInterfaceList = std::to_array({
    InterfaceOf<TIdentifyHandle<TarFsFile>, TarFsFile>(kOsIdentifyGuid),
    InterfaceOf<TFileHandle<TarFsFile>, TarFsFile>(kOsFileGuid),
});

OsStatus TarFsFile::query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) {
    return kFileInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus TarFsFile::interfaces(OsIdentifyInterfaceList *list) {
    return kFileInterfaceList.list(list);
}

OsStatus TarFsFile::read(ReadRequest request, ReadResult *result) {
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

OsStatus TarFsFile::stat(OsFileInfo *result) {
    size_t size = mHeader.getSize();

    *result = OsFileInfo {
        .LogicalSize = size,
        .BlockSize = kTarBlockSize,
        .BlockCount = sm::roundup(size, kTarBlockSize) / kTarBlockSize,
    };

    return OsStatusSuccess;
}

//
// tarfs folder implementation
//

static constexpr inline InterfaceList kFolderInterfaceList = std::to_array({
    InterfaceOf<TIdentifyHandle<TarFsFolder>, TarFsFolder>(kOsIdentifyGuid),
    InterfaceOf<TFolderHandle<TarFsFolder>, TarFsFolder>(kOsFolderGuid),
    InterfaceOf<TIteratorHandle<TarFsFolder>, TarFsFolder>(kOsIteratorGuid),
});

OsStatus TarFsFolder::query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) {
    return kFolderInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus TarFsFolder::interfaces(OsIdentifyInterfaceList *list) {
    return kFileInterfaceList.list(list);
}

//
// tarfs mount implementation
//

OsStatus TarFsMount::walk(const VfsPath& path, sm::RcuSharedPtr<INode> *folder) {
    if (path.segmentCount() == 1) {
        *folder = mRootNode;
        return OsStatusSuccess;
    }

    sm::RcuSharedPtr<INode> current = mRootNode;

    for (auto segment : path.parent()) {
        std::unique_ptr<IFolderHandle> folder;
        if (OsStatus status = OpenFolderInterface(current, nullptr, 0, std::out_ptr(folder))) {
            return status;
        }

        sm::RcuSharedPtr<INode> child = nullptr;
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

TarFsMount::TarFsMount(TarFs *tarfs, sm::RcuDomain *domain, sm::SharedPtr<km::IBlockDriver> block)
    : IVfsMount(tarfs, domain)
    , mBlock(block)
    , mMedia(mBlock.get())
    , mRootNode(sm::rcuMakeShared<TarFsFolder>(mDomain, TarEntry{}, nullptr, this))
{
    sm::AbslBTreeMap<VfsPath, TarEntry> headers;
    if (OsStatus status = ParseTar(&mMedia, TarParseOptions{}, &headers)) {
        TarLog.errorf("Failed to parse tar archive: ", OsStatusId(status));
        return;
    }

    for (auto& [path, header] : headers) {
        VfsNodeType type = header.header.getType();
        VfsStringView name = path.name();

        sm::RcuSharedPtr<INode> parent = nullptr;
        if (OsStatus status = walk(path, &parent)) {
            TarLog.warnf("Failed to find parent for '", path, "' : ", OsStatusId(status));
            continue;
        }

        sm::RcuSharedPtr<INode> node = nullptr;
        if (type == VfsNodeType::eFile) {
            node = sm::rcuMakeShared<TarFsFile>(mDomain, header, parent, this);
        } else if (type == VfsNodeType::eFolder) {
            node = sm::rcuMakeShared<TarFsFolder>(mDomain, header, parent, this);
        } else {
            TarLog.warnf("Invalid node type for '", path, "'");
            continue;
        }

        if (node == nullptr) {
            TarLog.warnf("Failed to create node for '", path, "'");
            continue;
        }

        std::unique_ptr<vfs::IFolderHandle> folder;
        if (OsStatus status = OpenFolderInterface(parent, nullptr, 0, std::out_ptr(folder))) {
            TarLog.warnf("Failed to query folder for '", path, "' : ", OsStatusId(status));
            continue;
        }

        if (OsStatus status = folder->mknode(name, node)) {
            TarLog.warnf("Failed to add folder '", path, "' : ", OsStatusId(status));
        }
    }
}

OsStatus TarFsMount::root(sm::RcuSharedPtr<INode> *node) {
    *node = mRootNode;
    return OsStatusSuccess;
}

//
// tarfs driver implementation
//

OsStatus TarFs::mount(sm::RcuDomain *, IVfsMount **) {
    // TODO: until I have a good solution for passing mount parameters
    // this will remain unimplemented.
    return OsStatusNotSupported;
}

OsStatus TarFs::unmount(IVfsMount *mount) {
    delete mount;
    return OsStatusSuccess;
}

OsStatus TarFs::createMount(IVfsMount **mount, sm::RcuDomain *domain, sm::SharedPtr<km::IBlockDriver> block) {
    TarFsMount *result = new(std::nothrow) TarFsMount(this, domain, block);
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
