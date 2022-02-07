#ifndef ASSERT_H
#define ASSERT_H

// an indirection is needed so __LINE__ is evaled before being concatenated.
#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)

#define CONCAT3(a, b, c) CONCAT(CONCAT(a, b), c)

// learned from http://www.pixelbeat.org/programming/gcc/static_assert.html
#ifdef __COUNTER__
// __COUNTER__ macro is define by GCC. Check doc here: https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html#Common-Predefined-Macros
#define static_assert(cond, ...) \
  enum { CONCAT(static_assert_enum_value__, __COUNTER__) = 1 / !!(cond), }
#else
// this version may fail if a single compilation unit redefines the static_assert_enum_value__ variable with the same line number multiple times.
//
// This happens e.g. if foo.c includes bar.h and both call static_assert at line X.
// The static_assert used in bar.h will include line number in file bar.h.
#define static_assert(cond, ...) \
  enum { CONCAT(static_assert_enum_value__, __LINE__) = 1 / !!(cond), }
#endif

// TODO: check NDEBUG?
// TODO: fall to dead loop in kernel model. We should just kill the process in user mode
#define assert(cond) do { \
  if (!(cond)) { \
    printf("%s:%d: Assertion fail: %s\n", __FILE__, __LINE__, #cond); \
    while (1) { \
    } \
  } \
} while (false)

#endif
