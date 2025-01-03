#include "display.hpp"

void km::DisplayTerminal::put(char c) {
    write(mCurrentColumn, mCurrentRow, c);
    buffer[mCurrentRow * kColumnCount + mCurrentColumn] = c;

    advance();
}

void km::DisplayTerminal::write(uint64_t x, uint64_t y, char c) {
    DrawCharacter(mDisplay, x, y, c, Pixel { 255, 255, 255 }, Pixel { 0, 0, 0 });
}

void km::DisplayTerminal::advance() {
    mCurrentColumn += 1;

    if (mCurrentColumn >= kColumnCount) {
        newline();
        mCurrentColumn = 0;
    }
}

void km::DisplayTerminal::newline() {
    mCurrentRow += 1;
    bool scroll = false;
    if (mCurrentRow >= kRowCount) {
        mCurrentRow = kRowCount - 1;
        scroll = true;
    }

    mCurrentColumn = 0;

    if (scroll) {
        memmove(buffer, buffer + kColumnCount, kColumnCount * (kRowCount - 1));
        memset(buffer + kColumnCount * (kRowCount - 1), ' ', kColumnCount);

        // redraw the screen
        for (uint64_t y = 0; y < kRowCount; y++) {
            for (uint64_t x = 0; x < kColumnCount; x++) {
                write(x, y, buffer[y * kColumnCount + x]);
            }
        }
    }
}

void km::DisplayTerminal::print(stdx::StringView message) {
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
