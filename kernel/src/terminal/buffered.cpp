#include "display.hpp"

static km::Canvas InMemoryCanvas(const km::Canvas& geometry, km::SystemMemory& memory) {
    uint8_t *data = (uint8_t*)memory.allocate(geometry.size(), x64::kPageSize);

    return km::Canvas(geometry, data);
}

km::BufferedTerminal::BufferedTerminal(km::Canvas display, SystemMemory& memory)
    : mCurrentColumn(0)
    , mCurrentRow(0)
    , mColumnCount(display.width() / 8)
    , mRowCount(display.height() / 8)
    , mDisplay(display)
    , mBackBuffer(InMemoryCanvas(display, memory))
    , mTextBuffer((char*)memory.allocate(bufferSize()))
{ }
