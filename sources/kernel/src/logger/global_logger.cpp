#include "logger/logger.hpp"
#include "panic.hpp"

void km::LogQueue::initGlobalLogQueue(uint32_t messageQueueCapacity) {
    OsStatus status = LogQueue::create(messageQueueCapacity, &GlobalLogQueue);
    KM_CHECK(status == OsStatusSuccess, "Failed to create global log queue");
}
