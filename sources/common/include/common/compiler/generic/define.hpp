#pragma once

#ifndef DIAGNOSTIC_PUSH
#   define DIAGNOSTIC_PUSH()
#endif

#ifndef DIAGNOSTIC_POP
#   define DIAGNOSTIC_POP()
#endif

#ifndef DIAGNOSTIC_IGNORE
#   define DIAGNOSTIC_IGNORE(name)
#endif

#define DIAGNOSTIC_BEGIN_IGNORE(name) \
    DIAGNOSTIC_PUSH() \
    DIAGNOSTIC_IGNORE(name)

#define DIAGNOSTIC_END_IGNORE() \
    DIAGNOSTIC_POP()
