#include "ktest.hpp"

static kmtest::Machine *gMachine = nullptr;

void kmtest::Machine::reset(kmtest::Machine *machine) {
    delete gMachine;
    gMachine = machine;
}

kmtest::Machine *kmtest::Machine::instance() {
    return gMachine;
}
