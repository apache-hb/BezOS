#include "display.hpp"

#include <memory>

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
{
    std::uninitialized_fill_n(mTextBuffer, bufferSize(), ' ');
}

void km::BufferedTerminal::put(char c) {
    write(mCurrentColumn, mCurrentRow, c);
    mTextBuffer[mCurrentRow * mColumnCount + mCurrentColumn] = c;

    advance();
}

void km::BufferedTerminal::write(uint64_t x, uint64_t y, char c) {
    DrawCharacter(mBackBuffer, x, y, c, Pixel { 255, 255, 255 }, Pixel { 0, 0, 0 });
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
        mCurrentRow = mRowCount - 1;
        scroll = true;
    }

    mCurrentColumn = 0;

    if (scroll) {
        memmove(mTextBuffer, mTextBuffer + mColumnCount, mColumnCount * (mRowCount - 1));
        memset(mTextBuffer + mColumnCount * (mRowCount - 1), ' ', mColumnCount);

        // redraw the screen
        for (uint64_t y = 0; y < mRowCount; y++) {
            for (uint64_t x = 0; x < mColumnCount; x++) {
                write(x, y, mTextBuffer[y * mColumnCount + x]);
            }
        }
    }

    flush();
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

void km::BufferedTerminal::flush() {
    for (uint16_t row = 0; row < mRowCount; row++) {
        // count the number of characters until the end of the line
        const char *begin = mTextBuffer + row * mColumnCount;
        const char *end = begin + mColumnCount;

        // do this by counting the number of trailing spaces
        while (end > begin && end[-1] == ' ') {
            end--;
        }

        if (end == begin)
            continue;

        size_t size = end - begin;

        // now copy all the lines for this row
        for (size_t i = 0; i < 8; i++) {
            uint64_t y = row * 8 + i;
            uint8_t *src = (uint8_t*)mBackBuffer.address() + y * mBackBuffer.pitch();
            uint8_t *dst = (uint8_t*)mDisplay.address() + y * mDisplay.pitch();
            size_t count = size * 8;
            memcpy(dst, src, count * mDisplay.bpp() / 8);

            // then copy black space for the rest of the line
            // memset(dst + count * mDisplay.bpp() / 8, 0, (mColumnCount - size) * 8 * mDisplay.bpp() / 8);
        }
    }
}
