#include "notify.hpp"

#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"
#include "log.hpp"

using namespace km;

using SynchronizedTlsfAllocator = mem::SynchronizedAllocator<mem::TlsfAllocator>;

static mem::IAllocator *gNotificationAllocator = nullptr;

void *km::NotificationQueueTraits::malloc(size_t size) {
    return gNotificationAllocator->allocate(size);
}

void km::NotificationQueueTraits::free(void *ptr) {
    gNotificationAllocator->deallocate(ptr, 0);
}

void km::InitAqAllocator(void *memory, size_t size) {
    KM_CHECK(memory != nullptr, "Invalid memory for notification allocator.");
    KM_CHECK(size > 0, "Invalid size for notification allocator.");
    KM_CHECK(gNotificationAllocator == nullptr, "Notification allocator already initialized.");

    gNotificationAllocator = new SynchronizedTlsfAllocator(memory, size);
}

OsStatus Topic::addNotification(sm::RcuDomain *domain, INotification *notification) {
    if (notification == nullptr) {
        KmDebugMessage("[AQ] Dropped notification due to memory exhaustion\n");
        return OsStatusOutOfMemory;
    }

    sm::RcuSharedPtr<INotification> ptr = {domain, notification};
    if (queue.enqueue(ptr)) {
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
    sm::RcuSharedPtr<INotification> notification = nullptr;
    size_t count = 0;

    stdx::SharedLock guard(mLock);

    while (queue.try_dequeue(notification)) {
        for (ISubscriber *subscriber : mSubscribers) {
            subscriber->notify(this, notification);
        }

        if (++count >= limit) {
            break;
        }
    }

    return count;
}

OsStatus NotificationStream::addNotification(Topic *topic, INotification *notification) {
    return topic->addNotification(&mDomain, notification);
}

Topic *NotificationStream::createTopic(sm::uuid id, stdx::String name) {
    stdx::UniqueLock guard(mTopicLock);

    auto [iter, ok] = mTopics.insert({ id, sm::makeUnique<Topic>(id, std::move(name)) });
    if (!ok) {
        KmDebugMessage("[AQ] Failed to create topic\n");
        return nullptr;
    }

    auto& [key, value] = *iter;
    KmDebugMessage("[AQ] Created topic '", value->name(), "'\n");
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
