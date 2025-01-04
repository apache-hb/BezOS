#include "display.hpp"

void km::Terminal::put(char c) {
    write(mCurrentColumn, mCurrentRow, c);
    advance();
}

void km::Terminal::write(uint64_t x, uint64_t y, char c) {
    DrawCharacter(mDisplay, x, y, c, Pixel { 255, 255, 255 }, Pixel { 0, 0, 0 });
}

void km::Terminal::advance() {
    mCurrentColumn += 1;

    if (mCurrentColumn >= mColumnCount) {
        newline();
        mCurrentColumn = 0;
    }
}

void km::Terminal::newline() {
    mCurrentRow += 1;
    bool scroll = false;
    if (mCurrentRow >= mRowCount) {
        mCurrentRow = 0;
        scroll = true;
    }

    mCurrentColumn = 0;

    if (scroll) {
        memset(mDisplay.address(), 0, mDisplay.size());
    }
}

void km::Terminal::print(stdx::StringView message) {
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
