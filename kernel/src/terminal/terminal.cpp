#include "display.hpp"

void km::DisplayTerminal::put(char c) {
    write(mCurrentColumn, mCurrentRow, c);
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
        mCurrentRow = 0;
        scroll = true;
    }

    mCurrentColumn = 0;

    if (scroll) {
        memset(mDisplay.address(), 0, mDisplay.size());
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
