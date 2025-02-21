#include "isr/isr.hpp"

#include "log.hpp"

km::IsrContext km::DefaultIsrHandler(km::IsrContext *context) {
    KmDebugMessage("[INT] Unhandled interrupt: ", context->vector, " Error: ", context->error, "\n");
    return *context;
}
