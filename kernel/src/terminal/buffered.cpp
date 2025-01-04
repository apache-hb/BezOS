#include "display.hpp"

static km::Canvas InMemoryCanvas(km::Canvas geometry, km::SystemMemory& memory) {
    size_t size = geometry.size();
    uint8_t *data = (uint8_t*)memory.allocate(size, x64::kPageSize);
    memset(data, 0, size);

    return km::Canvas(geometry, data);
}

km::BufferedTerminal::BufferedTerminal(km::Canvas display, SystemMemory& memory)
    : mDisplay(display)
    , mBackBuffer(InMemoryCanvas(display, memory))
    , mCurrentRow(0)
    , mCurrentColumn(0)
    , mRowCount(display.height() / 8)
    , mColumnCount(display.width() / 8)
{ }

void km::BufferedTerminal::put(char c) {
    write(mCurrentColumn, mCurrentRow, c);
    advance();
}

void km::BufferedTerminal::advance() {
    mCurrentColumn += 1;

    if (mCurrentColumn >= mColumnCount) {
        newline();
        mCurrentColumn = 0;
    }
}

void km::BufferedTerminal::newline() {
    mCurrentRow += 1;
    bool scroll = false;
    if (mCurrentRow >= mRowCount) {
        mCurrentRow = 0;
        scroll = true;
    }

    mCurrentColumn = 0;

    if (scroll) {
        memset(mDisplay.address(), 0, mDisplay.size());
    } else {
        // copy the current row from the back buffer to the display
        size_t rowSize = mDisplay.width() * 8 * (mDisplay.bpp() / 8);
        size_t offset = mCurrentRow * rowSize;
        memcpy((uint8_t*)mDisplay.address() + offset, (uint8_t*)mBackBuffer.address() + offset, rowSize);
    }
}

void km::BufferedTerminal::write(uint64_t x, uint64_t y, char c) {
    DrawCharacter(mBackBuffer, x, y, c, Pixel { 255, 255, 255 }, Pixel { 0, 0, 0 });
}

void km::BufferedTerminal::print(stdx::StringView message) {
    for (char c : message) {
        if (c == '\0') {
            continue;
        }

        if (c == '\n') {
            newline();
        } else {
            put(c);
        }
    }
}
