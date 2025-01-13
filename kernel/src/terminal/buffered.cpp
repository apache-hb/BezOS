#include "display.hpp"

km::BufferedTerminal::BufferedTerminal(Canvas display, Canvas backBuffer)
    : mDisplay(display)
    , mBackBuffer(backBuffer)
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
    }
}

void km::BufferedTerminal::newline() {
    mCurrentRow += 1;
    bool clear = false;
    if (mCurrentRow >= mRowCount) {
        mCurrentRow = 0;
        clear = true;
    }

    mCurrentColumn = 0;

    if (clear) {
        memset(mDisplay.address(), 0, mDisplay.size());
    } else {
        // copy the current row from the back buffer to the display
        size_t rowSize = mDisplay.stride() * 8;
        size_t offset = (mCurrentRow - 1) * rowSize;
        memcpy((uint8_t*)mDisplay.address() + offset, (uint8_t*)mBackBuffer.address() + offset, rowSize);

        // zero out the back buffer row
        memset((uint8_t*)mBackBuffer.address() + offset, 0, rowSize);
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

void km::BufferedTerminal::clear() {
    mDisplay.fill(Pixel { 0, 0, 0 });
    mBackBuffer.fill(Pixel { 0, 0, 0 });
    mCurrentRow = 0;
    mCurrentColumn = 0;
}
