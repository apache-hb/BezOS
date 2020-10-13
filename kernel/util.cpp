#include "util.h"

#include "log/log.h"

extern "C" void __cxa_pure_virtual() {
    log::fatal("pure virtual member function called");
    for (;;) { }
}
