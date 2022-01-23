#ifndef ASSERT_H
#define ASSERT_H

// an indirection is needed so __LINE__ is evaled before being concatenated.
#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)

// learned from http://www.pixelbeat.org/programming/gcc/static_assert.html
#define static_assert(cond, ...) \
  enum { CONCAT(static_assert_enum_value__, __LINE__) = 1 / !!(cond), }

#endif
