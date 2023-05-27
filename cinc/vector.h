/*
 * Call this vector.h rather than vector on purpose to distinguish with stl.
 *
 * I naively implement this based on my understanding of the behavior of std::vector.
 * Performance is not a consideration for now.
 */
#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#define INITIALIZE_CAPACITY 8

template <typename T>
class vector {
 public:
  vector() : data_(nullptr), capacity_(0), size_(0) { }
  void push_back(const T& val) {
    if (size_ >= capacity_) {
      uint32_t new_capa = data_ ? (capacity_ << 1) : INITIALIZE_CAPACITY;
      data_ = (T*) realloc(data_, new_capa * sizeof(T));

      capacity_ = new_capa;
    }

    assert(size_ < capacity_);
    data_[size_++] = val;
  }

  /*
   * To support destructor, we need implement the following symbols:
   * - /Users/shunting/Documents/sos/kernel/test_kernel.cpp:24: undefined reference to `_Unwind_Resume'
   * - i686-elf-ld: out/kernel/test_kernel.o:(.eh_frame+0x13): undefined reference to `__gxx_personality_v0'
   *
   * Skip for now. User of this class need explicitly call destruct
   */
  #if 0
  ~vector() {
  #else
  void destruct() {
  #endif
    if (data_) {
      free(data_);
      data_ = nullptr;
      capacity_ = 0;
      size_ = 0;
    }
  }

  uint32_t size() const { return size_; }
  uint32_t capacity() const { return capacity_; }

  T& operator[](int idx) {
    assert(idx >= 0 && idx < size_);
    return data_[idx];
  }
 private:
  T* data_;
  uint32_t capacity_;
  uint32_t size_; 
};
