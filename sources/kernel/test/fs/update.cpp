#include <gtest/gtest.h>
#include <latch>
#include <thread>

#include "fs/utils.hpp"
#include "fs/vfs.hpp"

using namespace vfs;

static std::string RandomString(int size, std::uniform_int_distribution<int8_t> dist, std::mt19937& mt) {
    std::string result;
    result.reserve(size);

    for (int i = 0; i < size; i++) {
        result.push_back(dist(mt));
    }

    return result;
}

TEST(VfsUpdateTest, Update) {
    vfs::VfsRoot vfs;

    {
        sm::RcuSharedPtr<INode> node = nullptr;
        OsStatus status = vfs.mkdir(BuildPath("test"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    std::vector<std::string> names;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int8_t> dist('a', 'z');
    for (int i = 0; i < 500; i++) {
        names.push_back(RandomString(10, dist, mt));
    }

    static constexpr size_t kThreadCount = 4;
    static constexpr size_t kItemCount = 10000;
    std::vector<std::jthread> threads;
    std::latch latch(kThreadCount + 1);

    std::atomic<int> createCount = 0;
    std::atomic<int> removeCount = 0;
    std::atomic<int> lookupCount = 0;

    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&, i]() {
            std::mt19937 mt2(0x1234 + i);
            std::uniform_int_distribution<int32_t> dist(0, names.size() - 1);

            latch.arrive_and_wait();

            // pick random elements from the in elements set and try to either create or remove files with that name.
            for (size_t j = 0; j < kItemCount; j++) {
                const std::string& name = names[dist(mt2)];

                sm::RcuSharedPtr<INode> node = nullptr;
                auto path = BuildPath("test", vfs::VfsStringView(name.data(), name.data() + name.size()));
                int action = mt2() % 3;
                if (action == 0) {
                    OsStatus status = vfs.lookup(path, &node);
                    ASSERT_TRUE(status == OsStatusSuccess || status == OsStatusNotFound);
                    lookupCount += (status == OsStatusSuccess) ? 1 : 0;
                } else if (action == 1) {
                    OsStatus status = vfs.create(path, &node);
                    ASSERT_TRUE(status == OsStatusSuccess || status == OsStatusAlreadyExists);
                    createCount += (status == OsStatusSuccess) ? 1 : 0;
                } else {
                    OsStatus status = vfs.lookup(path, &node);
                    if (status == OsStatusSuccess) {
                        OsStatus status = vfs.remove(node);
                        ASSERT_TRUE(status == OsStatusSuccess || status == OsStatusNotFound);
                        removeCount += (status == OsStatusSuccess) ? 1 : 0;
                    }
                }
            }
        });
    }

    threads.emplace_back([&](std::stop_token stop) {
        latch.arrive_and_wait();
        while (!stop.stop_requested()) {
            vfs.synchronize();
        }
    });

    threads.clear();
    ASSERT_NE(createCount, 0);
    ASSERT_NE(removeCount, 0);
    ASSERT_NE(lookupCount, 0);
    SUCCEED() << "Did not crash";
}

TEST(VfsUpdateTest, Iterate) {
    vfs::VfsRoot vfs;

    {
        sm::RcuSharedPtr<INode> node = nullptr;
        OsStatus status = vfs.mkdir(BuildPath("test"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    std::vector<std::string> names;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int8_t> dist('a', 'z');
    for (int i = 0; i < 500; i++) {
        names.push_back(RandomString(10, dist, mt));
    }

    static constexpr size_t kThreadCount = 4;
    static constexpr size_t kItemCount = 1000;
    std::vector<std::jthread> threads;
    std::latch latch(kThreadCount + 1);

    std::atomic<int> createCount = 0;
    std::atomic<int> removeCount = 0;
    std::atomic<int> lookupCount = 0;
    std::atomic<int> iterateCount = 0;
    std::atomic<int> nextCount = 0;

    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&, i]() {
            std::mt19937 mt2(0x1234 + i);
            std::uniform_int_distribution<int32_t> dist(0, names.size() - 1);

            latch.arrive_and_wait();

            // pick random elements from the in elements set and try to either create or remove files with that name.
            for (size_t j = 0; j < kItemCount; j++) {
                const std::string& name = names[dist(mt2)];

                sm::RcuSharedPtr<INode> node = nullptr;
                auto path = BuildPath("test", vfs::VfsStringView(name.data(), name.data() + name.size()));
                int action = mt2() % 4;
                if (action == 0) {
                    OsStatus status = vfs.lookup(path, &node);
                    ASSERT_TRUE(status == OsStatusSuccess || status == OsStatusNotFound);
                    lookupCount += (status == OsStatusSuccess) ? 1 : 0;
                } else if (action == 1) {
                    OsStatus status = vfs.create(path, &node);
                    ASSERT_TRUE(status == OsStatusSuccess || status == OsStatusAlreadyExists);
                    createCount += (status == OsStatusSuccess) ? 1 : 0;
                } else if (action == 2) {
                    OsStatus status = vfs.lookup(BuildPath("test"), &node);
                    ASSERT_TRUE(status == OsStatusSuccess);
                    iterateCount += (status == OsStatusSuccess) ? 1 : 0;
                    std::unique_ptr<vfs::IIteratorHandle> handle;
                    status = vfs::OpenIteratorInterface(node, nullptr, 0, std::out_ptr(handle));
                    ASSERT_EQ(status, OsStatusSuccess);

                    sm::RcuSharedPtr<INode> child = nullptr;
                    while (true) {
                        status = handle->next(&child);
                        if (status == OsStatusCompleted) {
                            break;
                        }

                        ASSERT_EQ(status, OsStatusSuccess);
                        ASSERT_NE(child, nullptr);
                        NodeInfo info = child->info();
                        ASSERT_NE(info.mount, nullptr);
                        ASSERT_NE(info.parent.lock(), nullptr);
                        ASSERT_EQ(info.name.count(), 10);

                        nextCount++;
                    }

                } else {
                    OsStatus status = vfs.lookup(path, &node);
                    if (status == OsStatusSuccess) {
                        OsStatus status = vfs.remove(node);
                        ASSERT_TRUE(status == OsStatusSuccess || status == OsStatusNotFound);
                        removeCount += (status == OsStatusSuccess) ? 1 : 0;
                    }
                }
            }
        });
    }

    threads.emplace_back([&](std::stop_token stop) {
        latch.arrive_and_wait();
        while (!stop.stop_requested()) {
            vfs.synchronize();
        }
    });

    threads.clear();
    ASSERT_NE(createCount, 0);
    ASSERT_NE(removeCount, 0);
    ASSERT_NE(lookupCount, 0);
    ASSERT_NE(iterateCount, 0);
    ASSERT_NE(nextCount, 0);
    SUCCEED() << "Did not crash";
}
