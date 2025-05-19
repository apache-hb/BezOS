#include "logger/logger.hpp"

void km::LogQueue::write(const LogMessageView& message) [[clang::nonreentrant]] {
    for (ILogAppender *appender : mAppenders) {
        appender->write(message);
    }
}

size_t km::LogQueue::writeAllMessages() {
    size_t count = 0;
    detail::LogMessage message;
    while (mQueue.tryPop(message)) {
        write({ message.location, message.message, message.logger, message.level });
        count++;
    }

    mComittedCount.fetch_add(count, std::memory_order_relaxed);

    return count;
}

OsStatus km::LogQueue::addAppender(ILogAppender *appender) noexcept {
    return mAppenders.add(appender);
}

void km::LogQueue::removeAppender(ILogAppender *appender) noexcept {
    mAppenders.remove_if(appender);
}

OsStatus km::LogQueue::recordMessage(detail::LogMessage message) noexcept [[clang::reentrant]] {
    if (mQueue.tryPush(message)) {
        return OsStatusSuccess;
    }

    mDroppedCount.fetch_add(1, std::memory_order_relaxed);

    return OsStatusOutOfMemory;
}

OsStatus km::LogQueue::submit(detail::LogMessage message) noexcept [[clang::reentrant]] {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

    if (mLock.try_lock()) {
        if (mQueue.isSetup()) writeAllMessages();
        write({ message.location, message.message, message.logger, message.level });
        mLock.unlock();
        return OsStatusSuccess;
    } else {
        return recordMessage(message);
    }

    CLANG_DIAGNOSTIC_POP();
}

size_t km::LogQueue::flush() [[clang::nonreentrant]] {
    stdx::LockGuard guard(mLock);
    return writeAllMessages();
}

OsStatus km::LogQueue::resizeQueue(uint32_t newCapacity) noexcept [[clang::allocating]] {
    return MessageQueue::create(newCapacity, &mQueue);
}

OsStatus km::LogQueue::create(uint32_t messageQueueCapacity, LogQueue *queue) noexcept [[clang::allocating]] {
    return MessageQueue::create(messageQueueCapacity, &queue->mQueue);
}

stdx::StringView km::Logger::getName() const noexcept [[clang::reentrant]] {
    return mName;
}

void km::Logger::submit(LogLevel level, stdx::StringView message, std::source_location location) noexcept [[clang::reentrant]] {
    detail::LogMessage logMessage {
        .level = level,
        .location = location,
        .logger = this,
        .message = message,
    };

    mQueue->submit(logMessage);
}

void km::Logger::dbg(stdx::StringView message, std::source_location location) noexcept [[clang::reentrant]] {
    submit(LogLevel::eDebug, message, location);
}

void km::Logger::info(stdx::StringView message, std::source_location location) noexcept [[clang::reentrant]] {
    submit(LogLevel::eInfo, message, location);
}

void km::Logger::warn(stdx::StringView message, std::source_location location) noexcept [[clang::reentrant]] {
    submit(LogLevel::eWarning, message, location);
}

void km::Logger::error(stdx::StringView message, std::source_location location) noexcept [[clang::reentrant]] {
    submit(LogLevel::eError, message, location);
}

void km::Logger::fatal(stdx::StringView message, std::source_location location) noexcept [[clang::reentrant]] {
    submit(LogLevel::eFatal, message, location);
}
