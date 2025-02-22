#pragma once

#include "std/spinlock.hpp"
#include "util/absl.hpp"

#include "std/rcuptr.hpp"
#include "std/string.hpp"
#include "std/queue.hpp"

#include "util/cxx_chrono.hpp"
#include "util/uuid.hpp"

namespace km {
    class NotificationStream;
    class INotification;
    class ISubscriber;

    class Topic {
        friend class NotificationStream;

        sm::uuid mId;
        stdx::String mName;

        moodycamel::ConcurrentQueue<sm::RcuSharedPtr<INotification>> queue;

        stdx::SharedSpinLock mLock;
        sm::FlatHashSet<ISubscriber*> mSubscribers;

        void addNotification(sm::RcuDomain *domain, INotification *notification);
        void subscribe(ISubscriber *subscriber);
        void unsubscribe(ISubscriber *subscriber);

    public:
        Topic(sm::uuid id, stdx::String name)
            : mId(id)
            , mName(std::move(name))
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
    };
}

template<>
struct std::hash<km::Topic> {
    size_t operator()(const km::Topic& topic) const noexcept {
        return std::hash<sm::uuid>()(topic.id());
    }
};

namespace km {
    using SendTime = std::chrono::utc_clock::time_point;

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

    class ISubscriber {
    public:
        virtual ~ISubscriber() = default;

        virtual void notify(Topic *topic, sm::RcuSharedPtr<INotification> notification) = 0;
    };

    class NotificationStream {
        using TopicSet = sm::FlatHashMap<sm::uuid, std::unique_ptr<Topic>>;

        sm::RcuDomain mDomain;

        stdx::SharedSpinLock mTopicLock;
        TopicSet mTopics;

        stdx::SpinLock mSubscriberLock;
        sm::FlatHashSet<std::unique_ptr<ISubscriber>> mSubscribers;

        void addNotification(Topic *topic, INotification *notification);

    public:
        UTIL_NOCOPY(NotificationStream);
        UTIL_NOMOVE(NotificationStream);

        NotificationStream() = default;

        template<std::derived_from<INotification> T, typename... Args>
        void publish(Topic *topic, Args&&... args) {
            auto notification = new T(std::forward<Args>(args)...);
            addNotification(topic, notification);
        }

        Topic *createTopic(sm::uuid id, stdx::String name);
        Topic *findTopic(sm::uuid id);

        ISubscriber *subscribe(Topic *topic, ISubscriber *subscriber);

        void unsubscribe(Topic *topic, ISubscriber *subscriber);

        size_t process(Topic *topic, size_t limit = 1024);

        size_t processAll(size_t limit = 1024);
    };
}
