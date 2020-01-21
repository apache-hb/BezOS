#ifndef COMMON_MACROS_H
#define COMMON_MACROS_H

#define CAT_INNER(l, r) l##r
#define CAT(l, r) CAT_INNER(l, r)

#define STATIC_ASSERT(expr) typedef char CAT(assertion, __COUNTER__)[(expr) ? 1 : -1]

#endif // COMMON_MACROS_H
