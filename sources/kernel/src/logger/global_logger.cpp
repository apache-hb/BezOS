#include "logger/logger.hpp"
#include "panic.hpp"

[[clang::no_destroy]]
static km::LogQueue gLogQueue{};

void km::LogQueue::initGlobalLogQueue(uint32_t messageQueueCapacity) {
    OsStatus status = LogQueue::create(messageQueueCapacity, &gLogQueue);
    KM_CHECK(status == OsStatusSuccess, "Failed to create global log queue");
}

km::Logger::Logger(stdx::StringView name)
    : Logger(name, &gLogQueue)
{ }
