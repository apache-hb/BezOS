#pragma once

#include <bezos/status.h>

namespace km {
    class PageTables;

    void copyHigherHalfMappings(PageTables *tables, const PageTables *source);
}
