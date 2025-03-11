#ifndef BEZOS_POSIX_ASSERT_H
#define BEZOS_POSIX_ASSERT_H 1

#include <detail/attributes.h>
#include <detail/cxx.h>

BZP_API_BEGIN

/**
 * @brief Assertion handler.
 *
 * @param expr The expression that failed.
 * @param file The file where the assertion failed.
 * @param line The line where the assertion failed.
 *
 * @note This function is not marked as noreturn or nothrow as user defined
 *       assertion handlers may throw exceptions or return to the calling code.
 */
extern void OsImplPosixAssert(const char *, const char *, unsigned) BZP_NONNULL(1, 2);

extern void OsImplPosixInstall(void(*)(const char *, const char *, unsigned)) BZP_NOEXCEPT BZP_NONNULL(1);

#ifdef NDEBUG
#   define assert(x) ((void)0)
#else
#   define assert(x) do { if (!(x)) { OsImplPosixAssert(#x, __FILE__, __LINE__); } } while (0)
#endif

BZP_API_END

#endif /* BEZOS_POSIX_ASSERT_H */
