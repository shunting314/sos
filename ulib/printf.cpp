#include <stdio.h>
#include <stdarg.h>
#include <syscall.h>

class CharBuffer {
 public:
  static CharBuffer& get() {
    // static local object does not work yet since it depends on __cxa_guard_acquire
    // TODO: support this later!
    // static CharBuffer __char_buffer_instance__;
    return __char_buffer_instance__;
  }

  void putchar(char ch) {
    if (pos_ == sizeof(buf_)) {
      flush();
    }
    buf_[pos_++] = ch;
  }

  void flush() {
    if (pos_ > 0) {
      write(1, buf_, pos_);
      pos_ = 0;
    }
  }
 private:
  static CharBuffer __char_buffer_instance__;
  char buf_[256];
  int pos_ = 0;
};

CharBuffer CharBuffer::__char_buffer_instance__;

void buffered_putchar(char ch) {
  CharBuffer::get().putchar(ch);
}

// defined in common/printf.cpp
typedef void putchar_fn_t(char ch);
int vprintf_int(const char*fmt, va_list va, putchar_fn_t* putchar_fn);

int printf(const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);

  int ret = vprintf_int(fmt, va, buffered_putchar);
  // flush anything remaining in the buffer
  CharBuffer::get().flush();
  return ret;
}

