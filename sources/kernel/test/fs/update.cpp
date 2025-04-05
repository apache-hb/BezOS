#include <gtest/gtest.h>
#include <latch>
#include <thread>

#include "fs2/vfs.hpp"

using namespace vfs2;

static std::string RandomString(int size, std::uniform_int_distribution<int8_t> dist, std::mt19937& mt) {
    std::string result;
    result.reserve(size);

    for (int i = 0; i < size; i++) {
        result.push_back(dist(mt));
    }

    return result;
}

TEST(VfsUpdateTest, Update) {
    vfs2::VfsRoot vfs;

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

            latch.arrive_and_wait();

            // pick random elements from the in elements set and try to either create or remove files with that name.
            std::vector<std::string> name;
            for (size_t j = 0; j < kItemCount; j++) {
                name.clear();
                std::sample(names.begin(), names.end(), std::back_inserter(name), 1, mt2);
                std::string_view nameView = name[0];

                sm::RcuSharedPtr<INode> node = nullptr;
                auto path = BuildPath("test", vfs2::VfsStringView(nameView.data(), nameView.data() + nameView.size()));
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
