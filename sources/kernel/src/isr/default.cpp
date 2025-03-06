#include "isr/isr.hpp"

#include "log.hpp"
#include "processor.hpp"

km::IsrContext km::DefaultIsrHandler(km::IsrContext *context) {
    KmDebugMessage("[INT] ", km::GetCurrentCoreId(), " Unhandled interrupt: ", context->vector, " Error: ", context->error, "\n");
    return *context;
}
