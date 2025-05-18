#include "notify.hpp"

#include "logger/logger.hpp"

using namespace km;

constinit static km::Logger AqLog { "AQ" };

OsStatus Topic::addNotification(INotification *notification) {
    if (notification == nullptr) {
        AqLog.warnf("Dropped notification due to memory exhaustion");
        return OsStatusOutOfMemory;
    }

    std::unique_ptr<INotification> ptr{notification};
    if (mQueue.tryPush(ptr)) {
        return OsStatusSuccess;
    } else {
        return OsStatusOutOfMemory;
    }
}

void Topic::subscribe(ISubscriber *subscriber) {
    stdx::UniqueLock guard(mLock);
    mSubscribers.insert(subscriber);
}

void Topic::unsubscribe(ISubscriber *subscriber) {
    stdx::UniqueLock guard(mLock);
    mSubscribers.erase(subscriber);
}

size_t Topic::process(size_t limit) {
    std::unique_ptr<INotification> notification = nullptr;
    size_t count = 0;

    stdx::SharedLock guard(mLock);

    while (mQueue.tryPop(notification)) {
        for (ISubscriber *subscriber : mSubscribers) {
            subscriber->notify(this, notification.get());
        }

        if (++count >= limit) {
            break;
        }
    }

    return count;
}

OsStatus NotificationStream::addNotification(Topic *topic, INotification *notification) {
    return topic->addNotification(notification);
}

Topic *NotificationStream::createTopic(sm::uuid id, stdx::String name, uint32_t capacity) {
    stdx::UniqueLock guard(mTopicLock);

    TopicQueue queue;
    if (OsStatus status = TopicQueue::create(capacity, &queue)) {
        AqLog.warnf("Failed to create topic queue: ", OsStatusId(status));
        return nullptr;
    }

    auto [iter, ok] = mTopics.insert({ id, sm::makeUnique<Topic>(id, std::move(name), std::move(queue)) });
    if (!ok) {
        AqLog.warnf("Failed to create topic ", name, ":", id);
        return nullptr;
    }

    auto& [key, value] = *iter;
    AqLog.dbgf("Created topic ", value->name(), ":", value->id());
    return value.get();
}

Topic *NotificationStream::findTopic(sm::uuid id) {
    stdx::SharedLock guard(mTopicLock);

    if (auto iter = mTopics.find(id); iter != mTopics.end()) {
        return iter->second.get();
    }

    return nullptr;
}

ISubscriber *NotificationStream::subscribe(Topic *topic, ISubscriber *subscriber) {
    stdx::LockGuard guard(mSubscriberLock);

    auto [iter, ok] = mSubscribers.emplace(subscriber);
    KM_CHECK(ok, "Failed to subscribe to topic");

    ISubscriber *ptr = iter->get();
    topic->subscribe(ptr);
    return ptr;
}

void NotificationStream::unsubscribe(Topic *topic, ISubscriber *subscriber) {
    topic->unsubscribe(subscriber);
}

size_t NotificationStream::processAll(size_t limit) {
    size_t count = 0;

    // TODO: make this thread safe

    stdx::SharedLock guard(mTopicLock);

    for (auto& [key, topic] : mTopics) {
        count += topic->process(limit - count);
        if (count >= limit) {
            break;
        }
    }

    return count;
}
