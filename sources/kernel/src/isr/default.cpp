#include "isr/isr.hpp"

#include "logger/categories.hpp"
#include "processor.hpp"

km::IsrContext km::DefaultIsrHandler(km::IsrContext *context) noexcept [[clang::reentrant]] {
    IsrLog.warnf(km::GetCurrentCoreId(), " Unhandled interrupt: ", context->vector, " Error: ", context->error);
    return *context;
}
