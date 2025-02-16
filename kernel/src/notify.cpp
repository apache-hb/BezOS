#include "notify.hpp"

using namespace km;

void Topic::addNotification(sm::RcuDomain *domain, INotification *notification) {
    sm::RcuSharedPtr<INotification> ptr = {domain, notification};
    queue.enqueue(ptr);
}

void Topic::subscribe(ISubscriber *subscriber) {
    subscribers.insert(subscriber);
}

void Topic::unsubscribe(ISubscriber *subscriber) {
    subscribers.erase(subscriber);
}

void NotificationStream::addNotification(Topic *topic, INotification *notification) {
    topic->addNotification(&mDomain, notification);
}

Topic *NotificationStream::createTopic(sm::uuid id, stdx::String name) {
    auto [iter, ok] = mTopics.insert({ id, sm::makeUnique<Topic>(id, std::move(name)) });
    if (!ok) {
        return nullptr;
    }

    auto& [key, value] = *iter;
    return value.get();
}

Topic *NotificationStream::findTopic(sm::uuid id) {
    if (auto iter = mTopics.find(id); iter != mTopics.end()) {
        return iter->second.get();
    }

    return nullptr;
}

ISubscriber *NotificationStream::subscribe(Topic *topic, ISubscriber *subscriber) {
    auto [iter, ok] = mSubscribers.emplace(subscriber);
    KM_CHECK(ok, "Failed to subscribe to topic");

    ISubscriber *ptr = iter->get();
    topic->subscribe(ptr);
    return ptr;
}

void NotificationStream::unsubscribe(Topic *topic, ISubscriber *subscriber) {
    topic->unsubscribe(subscriber);
}

size_t NotificationStream::process(Topic *topic, size_t limit) {
    sm::RcuSharedPtr<INotification> notification = nullptr;
    size_t count = 0;

    while (topic->queue.try_dequeue(notification) && count < limit) {
        for (ISubscriber *subscriber : topic->subscribers) {
            subscriber->notify(topic, notification);
        }

        count += 1;
    }

    return count;
}
