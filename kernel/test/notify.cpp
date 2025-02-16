#include <gtest/gtest.h>

#include "notify.hpp"

TEST(NotifyTest, Construct) {
    km::NotificationStream stream;
}

static constexpr sm::uuid kTestTopicId = sm::uuid::of("d79a9420-ec7d-11ef-97a0-00155d0cb009");

TEST(NotifyTest, CreateTopic) {
    km::NotificationStream stream;
    km::Topic *topic = stream.createTopic(kTestTopicId, "Test");
    ASSERT_NE(topic, nullptr);

    ASSERT_EQ(topic->id(), kTestTopicId);
    ASSERT_EQ(topic->name(), "Test");
}

TEST(NotifyTest, Subscribe) {
    km::NotificationStream stream;
    km::Topic *topic = stream.createTopic(kTestTopicId, "Test");
    ASSERT_NE(topic, nullptr);

    class TestNotification : public km::INotification {
        stdx::String mMessage;
    public:
        TestNotification(stdx::String message)
            : INotification(std::chrono::utc_clock::now())
            , mMessage(std::move(message))
        { }

        stdx::StringView message() const {
            return mMessage;
        }
    };

    class TestSubscriber : public km::ISubscriber {
        km::Topic *mTopic;

    public:
        TestSubscriber(km::Topic *topic)
            : mTopic(topic)
        { }

        void notify(km::Topic *topic, sm::RcuSharedPtr<km::INotification> notification) override {
            ASSERT_EQ(topic, mTopic);
            ASSERT_NE(notification, nullptr);

            TestNotification *test = dynamic_cast<TestNotification*>(notification.get());
            ASSERT_NE(test, nullptr);

            ASSERT_EQ(test->message(), "Test");
        }
    };

    km::ISubscriber *sub = stream.subscribe(topic, new TestSubscriber(topic));
    ASSERT_NE(sub, nullptr);

    stream.publish<TestNotification>(topic, "Test");
    stream.process(topic, 1);
}
