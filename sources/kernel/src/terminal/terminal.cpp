#include "display.hpp"

void km::DirectTerminal::put(char c) {
    write(mCurrentColumn, mCurrentRow, c);
    advance();
}

void km::DirectTerminal::write(uint64_t x, uint64_t y, char c) {
    DrawCharacter(mDisplay, x, y, c, Pixel { 255, 255, 255 }, Pixel { 0, 0, 0 });
}

void km::DirectTerminal::advance() {
    mCurrentColumn += 1;

    if (mCurrentColumn >= mColumnCount) {
        newline();
    }
}

void km::DirectTerminal::newline() {
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

void km::DirectTerminal::print(stdx::StringView message) {
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

void km::DirectTerminal::clear() {
    mDisplay.fill(Pixel { 0, 0, 0 });
    mCurrentRow = 0;
    mCurrentColumn = 0;
}
