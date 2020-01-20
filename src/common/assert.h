#ifndef COMMON_ASSERT_H
#define COMMON_ASSERT_H 1

#define STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#define ASSERT(expr, msg) if(!(expr)) { /* todo */ }

#endif //COMMON_ASSERT_H
