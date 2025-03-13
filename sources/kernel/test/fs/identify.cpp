#include <gtest/gtest.h>

#include "fs2/vfs.hpp"

using namespace vfs2;

static void CheckIdentify(INode *node) {
    // Must have identify interface.
    std::unique_ptr<IHandle> handle;
    OsStatus status = node->query(kOsIdentifyGuid, nullptr, 0, std::out_ptr(handle));
    ASSERT_EQ(OsStatusSuccess, status);

    IIdentifyHandle *identify = static_cast<IIdentifyHandle*>(handle.get());
    OsIdentifyInfo info;
    status = identify->identify(&info);
    ASSERT_EQ(OsStatusSuccess, status);

    struct ListQuery {
        uint32_t InterfaceCount;
        OsGuid InterfaceGuids[32];
    };

    ListQuery query;
    status = identify->interfaces(&query, sizeof(query));
    ASSERT_EQ(OsStatusSuccess, status);

    auto guidEqual = [](const OsGuid &lhs, const OsGuid &rhs) {
        return std::memcmp(&lhs, &rhs, sizeof(OsGuid)) == 0;
    };

    // Must have no duplicate interfaces.
    ASSERT_TRUE(std::adjacent_find(std::begin(query.InterfaceGuids), std::begin(query.InterfaceGuids) + query.InterfaceCount, guidEqual) == std::end(query.InterfaceGuids));

    // Must be at least 1 interface for identify.
    ASSERT_GE(query.InterfaceCount, 1);

    // Must be able to create all interfaces that are listed.
    for (uint32_t i = 0; i < query.InterfaceCount; i++) {
        OsGuid guid = query.InterfaceGuids[i];

        std::unique_ptr<IHandle> handle;
        status = node->query(guid, nullptr, 0, std::out_ptr(handle));
        ASSERT_EQ(OsStatusSuccess, status);
    }
}

TEST(VfsIdentifyTest, FileIdentify) {
    vfs2::VfsRoot vfs;

    {
        INode *node = nullptr;
        OsStatus status = vfs.create(BuildPath("inventory.txt"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    INode *node = nullptr;
    OsStatus status = vfs.lookup(BuildPath("inventory.txt"), &node);
    ASSERT_EQ(OsStatusSuccess, status);
    ASSERT_NE(nullptr, node);

    CheckIdentify(node);
}
