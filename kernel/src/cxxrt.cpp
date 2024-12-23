#include "kernel.hpp"

extern "C" void __cxa_pure_virtual() {
    KM_PANIC("Pure virtual function called.");
}
