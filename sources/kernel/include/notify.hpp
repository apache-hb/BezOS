#pragma once

#include "std/ringbuffer.hpp"
#include "std/shared_spinlock.hpp"
#include "std/spinlock.hpp"
#include "util/absl.hpp"

#include "std/rcuptr.hpp"
#include "std/string.hpp"

#include "util/uuid.hpp"

namespace km {
    class NotificationStream;
    class INotification;
    class ISubscriber;

    using SendTime = OsInstant;

    static constexpr uint32_t kDefaultTopicCapacity = 64;

    class INotification : public sm::RcuIntrusivePtr<INotification> {
        SendTime mInstant;

    protected:
        INotification(SendTime instant)
            : mInstant(instant)
        { }

    public:
        virtual ~INotification() = default;

        SendTime instant() const {
            return mInstant;
        }
    };

    using TopicQueue = sm::AtomicRingQueue<std::unique_ptr<INotification>>;

    class Topic {
        friend class NotificationStream;

        sm::uuid mId;
        stdx::String mName;

        stdx::SpinLock mQueueLock;

        TopicQueue mQueue;

        stdx::SharedSpinLock mLock;
        sm::FlatHashSet<ISubscriber*> mSubscribers;

        OsStatus addNotification(INotification *notification);
        void subscribe(ISubscriber *subscriber);
        void unsubscribe(ISubscriber *subscriber);

    public:
        Topic(sm::uuid id, stdx::String name, TopicQueue queue)
            : mId(id)
            , mName(std::move(name))
            , mQueue(std::move(queue))
        { }

        sm::uuid id() const {
            return mId;
        }

        stdx::StringView name() const {
            return mName;
        }

        constexpr friend bool operator==(const Topic& lhs, const Topic& rhs) {
            return lhs.mId == rhs.mId;
        }

        constexpr bool operator==(sm::uuid id) const {
            return mId == id;
        }

        size_t process(size_t limit);
    };
}

template<>
struct std::hash<km::Topic> {
    size_t operator()(const km::Topic& topic) const noexcept {
        return std::hash<sm::uuid>()(topic.id());
    }
};

namespace km {
    class ISubscriber {
    public:
        virtual ~ISubscriber() = default;

        virtual void notify(Topic *topic, INotification *notification) = 0;
    };

    class NotificationStream {
        using TopicSet = sm::FlatHashMap<sm::uuid, std::unique_ptr<Topic>>;

        sm::RcuDomain mDomain;

        stdx::SharedSpinLock mTopicLock;
        TopicSet mTopics;

        stdx::SpinLock mSubscriberLock;
        sm::FlatHashSet<std::unique_ptr<ISubscriber>> mSubscribers;

        OsStatus addNotification(Topic *topic, INotification *notification);

    public:
        UTIL_NOCOPY(NotificationStream);
        UTIL_NOMOVE(NotificationStream);

        NotificationStream() = default;

        template<std::derived_from<INotification> T, typename... Args>
        OsStatus publish(Topic *topic, Args&&... args) {
            return addNotification(topic, new (std::nothrow) T(std::forward<Args>(args)...));
        }

        Topic *createTopic(sm::uuid id, stdx::String name, uint32_t capacity = kDefaultTopicCapacity);
        Topic *findTopic(sm::uuid id);

        ISubscriber *subscribe(Topic *topic, ISubscriber *subscriber);

        void unsubscribe(Topic *topic, ISubscriber *subscriber);

        size_t processAll(size_t limit = 1024);
    };
}
